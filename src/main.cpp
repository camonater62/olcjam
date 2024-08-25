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

    int screenWidth = GetScreenWidth();
    int screenHeight = GetScreenHeight();

    SetConfigFlags(FLAG_VSYNC_HINT | FLAG_WINDOW_HIGHDPI | 0);

    SetTargetFPS(0);
    InitAudioDevice();
    InitWindow(screenWidth, screenHeight, "OSU Runner!");

    Font font = LoadFontEx("res/ShareTechMono-Regular.ttf", 32, 0, 250);

    Model cubemodel = LoadModelFromMesh(GenMeshCube(1, 1, 1));

    Color ground_color = GetColor(0xbfd0b9ff);
    Color wall_color = GetColor(0xcbb9d0ff);

    float walk_speed = 30.0f;
    float bounds_width = 3.0f;
    float blockheight = 0.5f;

    Camera3D camera = { 0 };
    camera.position = { 0, 1.f, 0 };
    camera.target = { 0, 0, 4 };
    camera.up = { 0, 1, 0 };
    camera.fovy = 70;
    camera.projection = CAMERA_PERSPECTIVE;

    Song *song = nullptr;
    osu::Beatmap *beatmap = nullptr;

    Music audio = { 0 };
    PlayMusicStream(audio);
    SetMusicVolume(audio, 0.25f);

    bool playinggame = false;
    float gameoffset = 0;
    EnableCursor();
    int songselection = 0;

    while (!WindowShouldClose()) {
        BeginDrawing();
        {
            ClearBackground(BLACK);
            if (playinggame) {
                if (IsKeyPressed(KEY_ESCAPE)) {
                    playinggame = false;
                    SetExitKey(KEY_ESCAPE);
                    EnableCursor();
                    StopMusicStream(audio);
                    UnloadMusicStream(audio);
                    continue;
                }

                float dt = GetFrameTime();
                float time = GetTime() - gameoffset;

                Vector3 target = Vector3Subtract(camera.target, camera.position);
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

                BeginMode3D(camera);
                {
                    bool found_current = false;
                    float prevtime = -1.0f;
                    float nexttime = -1.0f;
                    for (auto iter = beatmap->HitObjects().begin();
                         iter != beatmap->HitObjects().end(); iter++) {
                        const auto &ho = *iter;

                        if (time > (ho.Time() + ho.Slides() * ho.Length()) / 1000.0f + 10.0f) {
                            continue;
                        }
                        if (time + 30.0f < ho.Time() / 1000.0f) {
                            break;
                        }

                        if (iter != beatmap->HitObjects().begin() && ho.Time() > 1000.0f * time
                            && !found_current) {
                            const auto &prevobj = *(iter - 1);
                            prevtime = prevobj.Time() + prevobj.Length() * prevobj.Slides();
                            nexttime = ho.Time();
                            found_current = true;
                        }

                        if (ho.Type() == osu::HitObjectType::CIRCLE) {
                            Color color = ground_color;
                            float xpos = bounds_width / 2 * sin(ho.Time() / 100.0f);
                            float ypos = -blockheight;
                            float zpos = walk_speed * (ho.Time() / 1000.0f - time);
                            Vector3 pos = { xpos, ypos, zpos };
                            DrawCube(pos, 1.f, blockheight, 1.f, color);
                        } else {
                            Color color = wall_color;
                            Vector3 lastpos = Vector3Zero();
                            for (int i = 0; i < ho.Slides(); i++) {
                                Vector3 pos = Vector3Zero();
                                {
                                    int length = i * ho.Length();
                                    float endtime = ho.Time() + length;
                                    if (iter + 1 != beatmap->HitObjects().end()
                                        && endtime >= (iter + 1)->Time()) {
                                        // This block shouldn't be necessary, but I messed
                                        // something up and can't figure out what it is :(
                                        endtime = (iter + 1)->Time();
                                    }
                                    pos.x = bounds_width / 2 * sin(endtime / 100.0f);
                                    pos.y = -blockheight / 2.0f;
                                    pos.z = walk_speed * float(endtime - time * 1000) / 1000.0f;
                                }
                                Vector3 nextpos = Vector3Zero();
                                {
                                    int length = (i + 1) * ho.Length();
                                    float endtime = ho.Time() + length;
                                    if (iter + 1 != beatmap->HitObjects().end()
                                        && endtime >= (iter + 1)->Time()) {
                                        // This block shouldn't be necessary, but I messed
                                        // something up and can't figure out what it is :(
                                        endtime = (iter + 1)->Time();
                                    }
                                    nextpos.x = bounds_width / 2 * sin(endtime / 100.0f);
                                    nextpos.y = -blockheight / 2.0f;
                                    nextpos.z = walk_speed * float(endtime - time * 1000) / 1000.0f;
                                }
                                Vector3 sliderpos = Vector3Lerp(pos, nextpos, 0.5f);
                                sliderpos.y -= blockheight / 2;

                                Vector3 offset = Vector3Subtract(nextpos, pos);
                                float mag = Vector3Length(offset);
                                Vector3 normaloffset = Vector3Normalize(offset);

                                Vector3 positivez = { 0, 0, 1 };
                                float angle = RAD2DEG * Vector3Angle(normaloffset, positivez);
                                if (offset.x < 0) {
                                    angle = -angle;
                                }

                                Vector3 rotaxis = { 0, 1, 0 };
                                Vector3 scale = { 1, blockheight, mag };

                                DrawModelEx(cubemodel, sliderpos, rotaxis, angle, scale, color);
                            }
                        }
                    }

                    if (prevtime != -1.0f && 1000.0f * time > prevtime) {
                        float distance = (nexttime - prevtime) / 1000.0f;
                        float grav = -80.f;
                        float half = (nexttime - prevtime) / 1000.0f / 2;
                        float pz = time - prevtime / 1000.0f;
                        float halfy = 0.5f * grav * (half * half - distance * half);
                        if (halfy > 10) {
                            grav = -1.0f;
                        }
                        float ypos = 0.5f * grav * (pz * pz - distance * pz) + 1.0f;
                        float ydiff = ypos - camera.position.y;
                        camera.position.y += ydiff;
                        camera.target.y += ydiff;
                    }

                    float lineheight = -2 * blockheight;
                    float leftedge = 1.5f * bounds_width / 2;
                    float rightedge = -1.5f * bounds_width / 2;
                    DrawLine3D(
                        { leftedge, lineheight, -100 }, { leftedge, lineheight, 1000 }, GRAY);
                    DrawLine3D(
                        { rightedge, lineheight, -100 }, { rightedge, lineheight, 1000 }, GRAY);
                }
                EndMode3D();

                DrawFPS(10, 10);

            } else {
                if (IsKeyPressed(KEY_DOWN) || IsKeyPressedRepeat(KEY_DOWN)) {
                    songselection += 1;
                } else if (IsKeyPressed(KEY_UP) || IsKeyPressedRepeat(KEY_UP)) {
                    songselection -= 1;
                }

                int total = 0;
                for (const auto &s : songs) {
                    for (const auto &m : s.maps) {
                        total++;
                    }
                }
                songselection = clamp(songselection, 0, total - 1);

                float baseoffset = -(songselection - 2) * 64;
                float offset = 0;
                int index = 0;
                for (auto &osusong : songs) {
                    for (auto &map : osusong.maps) {
                        DrawTextEx(font, format("{} [{}]", osusong.name, map.Version()).c_str(),
                            { 30, baseoffset + offset }, 64, 0, RAYWHITE);

                        if (index == songselection) {
                            song = &osusong;
                            beatmap = &map;

                            DrawCircle(15, baseoffset + offset + 32, 10, RED);
                        }

                        offset += 64;
                        index++;
                    }
                }

                if (IsKeyPressed(KEY_ENTER)) {
                    SetExitKey(KEY_NULL);
                    DisableCursor();
                    std::string folder = song->folderpath + "/";
                    audio = LoadMusicStream((folder + beatmap->AudioFilename()).c_str());
                    SetMusicVolume(audio, 0.25f);
                    PlayMusicStream(audio);
                    gameoffset = GetTime();
                    playinggame = true;
                    SetWindowTitle(format("{} - {}", beatmap->Artist(), beatmap->Title()).c_str());
                }
            }
        }
        EndDrawing();
    }

    UnloadFont(font);
    UnloadMusicStream(audio);
    CloseWindow();
    CloseAudioDevice();

    return 0;
}