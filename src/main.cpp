#include "osu.hpp"

#include <cstring>
#include <iostream>
using namespace std;

#include <raygui.h>
#include <raylib.h>
#include <raymath.h>
#include <rlgl.h>

#define FMT_HEADER_ONLY
#include <fmt/format.h>
using namespace fmt;

struct Song {
    string folderpath;
    string name;
    vector<osu::Beatmap> maps;
};

int main() {
    vector<Song> songs;

    FilePathList songfolders = LoadDirectoryFiles("res/songs");
    for (int i = 0; i < songfolders.count; i++) {
        Song song;

        const char *songpath = songfolders.paths[i];

        FilePathList difficulties = LoadDirectoryFilesEx(songpath, ".osu", false);
        for (int j = 0; j < difficulties.count; j++) {
            song.maps.emplace_back(difficulties.paths[j]);
        }
        UnloadDirectoryFiles(difficulties);

        song.folderpath = songpath;
        song.name = format("{} - {}", song.maps[0].Artist(), song.maps[0].Title());

        songs.push_back(song);
    }
    UnloadDirectoryFiles(songfolders);

    // for (const auto &song : songs) {
    //     println("{}", song.name);
    //     for (const auto &map : song.maps) {
    //         println("\t{}", map.Version());
    //     }
    // }

    int screenWidth = GetScreenWidth();
    int screenHeight = GetScreenHeight();

    SetConfigFlags(FLAG_VSYNC_HINT | FLAG_WINDOW_HIGHDPI | FLAG_FULLSCREEN_MODE);

    SetTargetFPS(0);
    InitAudioDevice();
    InitWindow(screenWidth, screenHeight, "OSU Runner!");

    Model cubemodel = LoadModelFromMesh(GenMeshCube(1, 1, 1));

    Color ground_color = GetColor(0xbfd0b9ff);
    Color wall_color = GetColor(0xcbb9d0ff);

    float walk_speed = 15.0f;

    Camera3D camera = { 0 };
    camera.position = { 0, 1.f, 0 };
    camera.target = { 0, 0, 4 };
    camera.up = { 0, 1, 0 };
    camera.fovy = 70;
    camera.projection = CAMERA_PERSPECTIVE;

    std::string folder = songs[0].folderpath + "/";

    osu::Beatmap &beatmap = songs[0].maps[0];
    cout << beatmap << endl;

    SetWindowTitle(format("{} - {}", beatmap.Artist(), beatmap.Title()).c_str());
    std::string audiofile = folder + beatmap.AudioFilename();
    Music audio = LoadMusicStream(audiofile.c_str());
    PlayMusicStream(audio);
    SeekMusicStream(audio, GetTime());

    DisableCursor();

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();

        // if (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT)) {
        //     camera.position.x += dt * walk_speed;
        //     camera.target.x += dt * walk_speed;
        // }
        // if (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT)) {
        //     camera.position.x -= dt * walk_speed;
        //     camera.target.x -= dt * walk_speed;
        // }

        Vector3 target = Vector3Subtract(camera.target, camera.position);
        // target.x *= -0.15f;
        target.y = 0;
        target = Vector3Normalize(target);

        UpdateCameraPro(&camera,
            {
                target.z * 20.0f * dt,
                target.x * -20.0f * dt,
                0,
            },
            {
                GetMouseDelta().x * 0.05f, // Rotation: yaw
                GetMouseDelta().y * 0.05f, // Rotation: pitch
                0.0f // Rotation: roll
            },
            GetMouseWheelMove() * 2.0f); // Move to target (zoom)
        camera.position.z = 0;
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
                            if (i > 0) {
                                Vector3 sliderpos = Vector3Lerp(pos, lastpos, 0.5f);
                                sliderpos.y -= blockheight / 2;

                                Vector3 offset = Vector3Subtract(pos, lastpos);
                                float mag = Vector3Length(offset);
                                Vector3 normaloffset = Vector3Normalize(offset);

                                Vector3 posz = { 0, 0, 1 };
                                float angle = RAD2DEG * Vector3Angle(normaloffset, posz);
                                if (offset.x < 0) {
                                    angle = -angle;
                                }

                                Vector3 rotaxis = { 0, 1, 0 };
                                Vector3 scale = { 1, blockheight, mag };

                                DrawModelEx(cubemodel, sliderpos, rotaxis, angle, scale, color);
                            }

                            lastpos = pos;
                        }
                    }
                }
            }
            EndMode3D();

            DrawFPS(10, 10);
        }
        EndDrawing();
    }

    UnloadMusicStream(audio);
    CloseWindow();
    CloseAudioDevice();

    return 0;
}