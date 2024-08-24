#include "osu.hpp"

#include <cstring>
#include <iostream>
#include <raylib.h>
#include <raymath.h>
#include <rlgl.h>

#define FMT_HEADER_ONLY
#include <fmt/format.h>

using namespace std;

int main() {
    int screenWidth = GetScreenWidth();
    int screenHeight = GetScreenHeight();

    SetConfigFlags(FLAG_VSYNC_HINT | FLAG_WINDOW_HIGHDPI | FLAG_FULLSCREEN_MODE);
    SetTargetFPS(0);
    InitAudioDevice();
    InitWindow(screenWidth, screenHeight, "OSU Runner!");

    const char *cwd = GetWorkingDirectory();
    ChangeDirectory("res/songs");

    FilePathList files = LoadDirectoryFiles("."); // LoadDirectoryFilesEx(".", ".osu", true);
    for (int i = 0; i < files.count; i++) {
        cout << files.paths[i] << endl;
    }
    ChangeDirectory(cwd);

    Color ground_color = GetColor(0xbfd0b9ff);
    Color wall_color = GetColor(0xcbb9d0ff);

    float walk_speed = 15.0f;

    Camera3D camera = { 0 };
    camera.position = { 0, 1.f, 0 };
    camera.target = { 0, 0, 4 };
    camera.up = { 0, 1, 0 };
    camera.fovy = 70;
    camera.projection = CAMERA_PERSPECTIVE;

    std::string folder = "res/songs/1294750 S3RL feat. Kayliana - You Are Mine/";
    std::string osufile = folder + "S3RL feat. Kayliana - You Are Mine (Nuvolina) [I'm yours].osu";

    osu::Beatmap beatmap(osufile);
    cout << beatmap << endl;

    SetWindowTitle(fmt::format("{} - {}", beatmap.Artist(), beatmap.Title()).c_str());
    std::string audiofile = folder + beatmap.AudioFilename();
    Music audio = LoadMusicStream(audiofile.c_str());
    PlayMusicStream(audio);
    SeekMusicStream(audio, GetTime());

    std::string backgroundfile = folder + beatmap.BackgroundFilename();
    Texture2D background = LoadTexture(backgroundfile.c_str());

    while (!WindowShouldClose()) {

        float dt = GetFrameTime();

        if (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT)) {
            camera.position.x += dt * walk_speed;
            camera.target.x += dt * walk_speed;
        }
        if (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT)) {
            camera.position.x -= dt * walk_speed;
            camera.target.x -= dt * walk_speed;
        }

        UpdateCamera(&camera, CAMERA_CUSTOM);

        UpdateMusicStream(audio);

        BeginDrawing();
        {
            ClearBackground(BLACK);

            BeginMode3D(camera);
            {
                for (const auto &ho : beatmap.HitObjects()) {
                    float blockheight = 0.5f;

                    if (GetTime() > (ho.Time() + ho.Slides() * ho.Length()) / 1000.0f + 10.0f) {
                        continue;
                    }
                    if (GetTime() + 30.0f < ho.Time() / 1000.0f) {
                        break;
                    }
                    if (ho.Type() == osu::HitObjectType::CIRCLE) {
                        Color color = ground_color;
                        float xpos = 2.0f * sin(ho.Time() / 100.0f);
                        float ypos = -blockheight;
                        float zpos = 50 * float(ho.Time() - GetTime() * 1000) / 1000.0f;
                        Vector3 pos = { xpos, ypos, zpos };
                        DrawCube(pos, 1.f, blockheight, 1.f, color);
                        // DrawCylinder(pos, 0.5f, 0.5f, blockheight, 20, color);
                    } else {
                        Color color = wall_color;
                        Vector3 lastpos = Vector3Zero();
                        for (int i = 0; i <= ho.Slides(); i++) {
                            int length = i * ho.Length();
                            float time = ho.Time() + length;
                            float xpos = 2.0f * sin(time / 100.0f);
                            float ypos = -blockheight / 2.0f;
                            float zpos = 50 * float(time - GetTime() * 1000) / 1000.0f;
                            Vector3 pos = { xpos, ypos, zpos };
                            // DrawCube(pos, 1.f, blockheight + 0.002f, 1.f,
                            //     ColorBrightness(wall_color, -0.2f));
                            if (i > 0) {
                                rlPushMatrix();
                                {
                                    rlTranslatef(lastpos.x, lastpos.y, lastpos.z);
                                    Vector3 offset = Vector3Subtract(pos, lastpos);
                                    Vector3 normaloffset = Vector3Normalize(offset);
                                    Vector3 posz = { 0, 0, 1 };

                                    float angle = Vector3Angle(normaloffset, posz);
                                    if (offset.x < 0) {
                                        angle = -angle;
                                    }

                                    float mag = Vector3Length(offset);
                                    rlRotatef(RAD2DEG * angle, 0, 1, 0);
                                    Vector3 sliderpos = { 0, 0, mag / 2 };
                                    DrawCube(sliderpos, 1.f, blockheight, mag, color);
                                }
                                rlPopMatrix();
                            }

                            lastpos = pos;
                        }
                    }
                }

                Vector3 ind_pos = { camera.position.x, 10, 100 };
                DrawBillboard(camera, background, ind_pos, 20, DARKGRAY);
            }
            EndMode3D();

            DrawFPS(10, 10);
        }
        EndDrawing();
    }

    UnloadMusicStream(audio);
    UnloadTexture(background);
    CloseWindow();
    CloseAudioDevice();

    return 0;
}