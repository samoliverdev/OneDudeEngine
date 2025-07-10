#pragma once
#include <OD/Core/Module.h>
#include <OD/Terrain/Heightmap.h>
#include <OD/Scene/Scene.h>

using namespace OD;

struct TerrainSample: OD::Module {
    Entity camera;

    Ref<Heightmap> GenerateHeightmap(int mapWidth, int mapHeight, int seed, float scale, int octaves, float persistance, float lacunarity, Vector2 offset);
    
    void OnInit() override;
    void OnUpdate(float deltaTime) override; 
    void OnRender(float deltaTime) override;
    void OnGUI() override;
    void OnResize(int width, int height) override;
    void OnExit() override;
};