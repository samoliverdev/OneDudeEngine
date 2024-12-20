#pragma once

#include <OD/OD.h>
//#include <OD/RenderPipeline/StandRenderPipeline2.h>
#include "Ultis/CameraMovement.h"
#include <assert.h>
#include "Ultis/Ultis.h"
#include "Ultis/FastNoiseLiteCpp.h"

using namespace OD;

struct TerrainSample: OD::Module {
    Entity camera;

    Ref<Heightmap> GenerateHeightmap(int mapWidth, int mapHeight, int seed, float scale, int octaves, float persistance, float lacunarity, Vector2 offset){
        //auto noise = fnlCreateState();
        //noise.noise_type = FNL_NOISE_VALUE;

        FastNoiseLite noise2;
        noise2.SetSeed(seed);
        noise2.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
        noise2.SetFractalType(FastNoiseLite::FractalType_FBm);
        noise2.SetFractalOctaves(octaves);
        noise2.SetFractalLacunarity(lacunarity);
        noise2.SetFractalGain(persistance);

        Random prng(seed);
        std::vector<Vector2> octaveOffsets(octaves); 
        for(int i = 0; i < octaves; i++){
            float offsetX = (float)prng.Range(-100000, 100000);
            float offsetY = (float)prng.Range(-100000, 100000);
            octaveOffsets[i] = Vector2(offsetX + offset.x, offsetY + offset.y);
        }

        Ref<Heightmap> noiseMap = CreateRef<Heightmap>(mapWidth, mapHeight);

        if(scale <= 0) scale = 0.0001f;

        float maxNoiseHeight = FLT_MIN;
        float minNoiseHeight = FLT_MAX;

        float halfWidth = mapWidth / 2.0f;
        float halfHeight = mapHeight / 2.0f;

        for(int y = 0; y < mapHeight; y++){
            for(int x = 0; x < mapWidth; x++){
                noiseMap->Set(x, y, noise2.GetNoise((x+offset.x)*scale, (y+offset.y)*scale) * 0.5f + 0.5f);
            }
        }

        for(int y = 0; y < mapHeight; y++){
            for(int x = 0; x < mapWidth; x++){
                //noiseMap->Set(x, y, InverseLerp(minNoiseHeight, maxNoiseHeight, noiseMap->Get(x, y)));
                //noiseMap->Set(x, y, Remap(noiseMap->Get(x, y), Vector2(-1,1), Vector2(0,1)));
            }
        }

        return noiseMap;
    }

    void OnInit() override {
        LogInfo("%sGame Init %s", "\033[0;32m", "\033[0m");
        Application::Vsync(false);
        SceneManager::Get().RegisterScript<CameraMovementScript>("CameraMovementScript");

        Scene* scene = SceneManager::Get().NewScene();

        Entity env = scene->AddEntity("Env");
        env.AddComponent<EnvironmentComponent>().settings.ambient = Color{0.11f, 0.16f, 0.25f, 1};

        Entity light = scene->AddEntity("Light");
        LightComponent& lightComponent = light.AddComponent<LightComponent>();
        lightComponent.color = {1,1,1};
        light.GetComponent<TransformComponent>().Position(Vector3(-2, 4, -1));
        light.GetComponent<TransformComponent>().LocalEulerAngles(Vector3(45, -125, 0));
        lightComponent.renderShadow = true;

        camera = scene->AddEntity("Camera");
        CameraComponent& cam = camera.AddComponent<CameraComponent>();
        camera.GetComponent<TransformComponent>().LocalPosition(Vector3(-37.4206, 0, 38.0931));
        camera.GetComponent<TransformComponent>().LocalEulerAngles(Vector3(0, 0, 0));
        camera.AddComponent<ScriptComponent>().AddScript<CameraMovementScript>()->moveSpeed = 160;
        cam.farClipPlane = 10000;
        cam.fieldOfView = 60;
  
        Entity terrain = scene->AddEntity("Terrain");
        TerrainComponent& terrainComponent = terrain.AddComponent<TerrainComponent>();
        //terrainComponent.terrainWidth = 1000;
        //terrainComponent.terrainLength = 500;
        terrainComponent.terrainHeight = 100;
        int heightmapSize = 1024 * 1;
        terrainComponent.SetHeightmap(
            GenerateHeightmap(heightmapSize, heightmapSize, 50, 0.25f/(4*1), 4, 0.5f, 2.0f, Vector2(0, 0))
        );
        terrainComponent.splatmap = AssetManager::Get().LoadAsset<Texture2D>("Sandbox/Textures/rgb-splat-map.png");
        terrainComponent.layer0 = AssetManager::Get().LoadAsset<Texture2D>("Sandbox/Textures/block.png");
        terrainComponent.layer1 = AssetManager::Get().LoadAsset<Texture2D>("Sandbox/Textures/brickwall.jpg");
        terrainComponent.layer2 = AssetManager::Get().LoadAsset<Texture2D>("Sandbox/Textures/floor.jpg");
        terrainComponent.layer3 = AssetManager::Get().LoadAsset<Texture2D>("Sandbox/Textures/Rock.jpg");
        terrainComponent.layer4 = AssetManager::Get().LoadAsset<Texture2D>("Sandbox/Textures/block.png");
    
        Application::AddModule<Editor>();
        //scene->Start();

        struct A{
            void print(){}
        };
        
        struct B{
            inline void Print(){ a.print(); }
        private:
            A a;
        };
        B b;
        b.Print();
    }

    void OnUpdate(float deltaTime) override {
        if(Input::IsKeyDown(KeyCode::T)) RenderContext::GetSettings().enableWireframe = !RenderContext::GetSettings().enableWireframe;
        if(Input::IsKeyDown(KeyCode::Y)) RenderContext::GetSettings().enableGizmosRuntime = !RenderContext::GetSettings().enableGizmosRuntime;
    }   

    void OnRender(float deltaTime) override {}

    void OnGUI() override {}
    void OnResize(int width, int height) override {}
    void OnExit() override {}
};