#include "osu.hpp"

#include <raylib.h>
#include <glm/glm.hpp>
#include <cstring>
#include <iostream>

#define FMT_HEADER_ONLY
#include <fmt/format.h>

using namespace glm;
using namespace std;

#pragma region Helpers

Matrix matToMatrix(mat4x4 m4) {
    Matrix m = {0};
    memcpy(&m, &m4, 16 * sizeof(float));
    return m;
}

mat4 matrixToMat(Matrix m) {
    mat4 m4 = {0};
    memcpy(&m4, &m, 16 * sizeof(float));
    return m4;
}

#pragma endregion

#pragma region main

int main()
{
    int screenWidth = 2560;
    int screenHeight = 1440;

    SetConfigFlags(FLAG_WINDOW_HIGHDPI | FLAG_VSYNC_HINT);
    SetTargetFPS(0);
    InitAudioDevice();
    InitWindow(screenWidth, screenHeight, "OSU Runner!");

    const char *cwd = GetWorkingDirectory();
    ChangeDirectory("res/songs");
    FilePathList files = LoadDirectoryFilesEx(".", ".osu", true);
    for (int i = 0; i < files.count; i++) {
        cout << files.paths[i] << endl;
    }
    ChangeDirectory(cwd);

    Color ground_color = GetColor(0xbfd0b9ff);
    Color wall_color = GetColor(0xcbb9d0ff);

    float walk_speed = 15.0f;

    Camera3D camera = {0};
    camera.position = {0, 1.f, 0};
    camera.target = {0, 0, 4};
    camera.up = {0, 1, 0};
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

    while (!WindowShouldClose())
    {

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
                // DrawGrid(1000, 1);

                for (const auto& ho : beatmap.HitObjects()) {
                    float blockheight = 0.5f;
                    float xpos = 2.0f * glm::sin(ho.Time() / 100.0f);
                    float ypos = -blockheight / 2.0f;
                    float zpos = 50 * float(ho.Time() - GetTime() * 1000) / 1000.0f;

                    if (zpos < -10) {
                        continue;
                    }
                    if (zpos > 200) {
                        break;
                    }
                    Vector3 pos = {xpos, ypos, zpos};
                    Color color = (ho.Type() == osu::HitObjectType::CIRCLE) ? ground_color : wall_color;
                    DrawCube(pos, 1.f, blockheight, 1.f, color);
                }

                Vector3 ind_pos = {camera.position.x, 10, 100};
                DrawBillboard(camera, background, ind_pos, 20, DARKGRAY);
            }
            EndMode3D();

            DrawFPS(10, 10);
        }
        EndDrawing();
    }

    CloseWindow();
    CloseAudioDevice();

    return 0;
}

#pragma endregion // main