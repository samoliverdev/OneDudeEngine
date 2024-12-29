#pragma once
#include <OD/OD.h>

using namespace OD;

struct ProceduralTerrain2: public OD::Module{
    Scene* scene;
    Entity camera;
    Entity terrain;

    int seed;
    float scale;
    int octaves;
    float persistance;
    float lacunarity;
    Vector2 offset;

    int heightmapSize;

    Ref<Heightmap> GenerateHeightmap(int mapWidth, int mapHeight, int seed, float scale, int octaves, float persistance, float lacunarity, Vector2 offset);

    void OnInit() override;
    void OnUpdate(float deltaTime) override;
    void OnRender(float deltaTime) override;
    void OnGUI() override;
    void OnResize(int width, int height) override;
    void OnExit() override;
};