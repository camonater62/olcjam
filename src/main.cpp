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


    Color ground_color = GetColor(0xbfd0b9ff);
    Color wall_color = GetColor(0xcbb9d0ff);

    Camera3D camera = {0};
    camera.position = {0, 1, 0};
    camera.target = {0, 1, 1};
    camera.up = {0, 1, 0};
    camera.fovy = 60;
    camera.projection = CAMERA_PERSPECTIVE;

    osu::Beatmap beatmap("res/1470072 Mage - The Words I Never Said/Mage - The Words I Never Said (Strategas) [Regret].osu.txt");
    cout << beatmap << endl;

    InitAudioDevice();
    Music audio = LoadMusicStream("res/1470072 Mage - The Words I Never Said/audio.wav");
    PlayMusicStream(audio);

    InitWindow(screenWidth, screenHeight, "OSU Runner!");
    SetWindowTitle(fmt::format("{} - {}", beatmap.Artist(), beatmap.Title()).c_str());

    while (!WindowShouldClose())
    {
        UpdateMusicStream(audio);

        UpdateCamera(&camera, CAMERA_FIRST_PERSON);

        BeginDrawing();
        {
            ClearBackground(BLACK);

            BeginMode3D(camera);
            {
                DrawGrid(1000, 1);

                for (const auto& ho : beatmap.HitObjects()) {
                    float blockheight = 0.5f;
                    float xpos = 2.0f * glm::sin(ho.Time() / 100.0f);
                    float ypos = -blockheight / 2.0f;
                    float zpos = 50 * float(ho.Time() - GetMusicTimePlayed(audio) * 1000) / 1000.0f;
                    Vector3 pos = {xpos, ypos, zpos};
                    Color color = (ho.Type() == osu::HitObjectType::CIRCLE) ? ground_color : wall_color;
                    DrawCube(pos, 1.f, blockheight, 1.f, color);
                }

                Vector3 ind_pos = camera.position;
                ind_pos.z += 100;
                DrawSphere(ind_pos, 1, RAYWHITE);
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