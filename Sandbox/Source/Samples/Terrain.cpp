#include "OD/pch.h"
#include "Terrain.h"
#include <OD/Scene/SceneManager.h>
#include <OD/Graphics/Model.h>
#include <OD/Graphics/Shader.h>
#include <OD/RenderPipeline/CameraComponent.h>
#include <OD/RenderPipeline/LightComponent.h>
#include <OD/RenderPipeline/EnvironmentComponent.h>
#include <OD/RenderPipeline/ModelRendererComponent.h>
#include <OD/RenderPipeline/RenderContext.h>
#include <OD/Terrain/Terrain.h>
#include <OD/Editor/Editor.h>
#include <OD/Core/Application.h>
#include <OD/Core/Input.h>
#include "Ultis/CameraMovement.h"
#include "Ultis/Ultis.h"
#include "Standard/Ultis/FastNoiseLiteCpp.h"
#include "Physics.h"
#include <assert.h>

Ref<Heightmap> TerrainSample::GenerateHeightmap(int mapWidth, int mapHeight, int seed, float scale, int octaves, float persistance, float lacunarity, Vector2 offset){
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

void TerrainSample::OnInit(){
    LogInfo("%sGame Init %s", "\033[0;32m", "\033[0m");
    Application::Vsync(false);
    SceneManager::Get().RegisterScript<CameraMovementScript>("CameraMovementScript");
    SceneManager::Get().RegisterScript<PhysicsCubeS>("PhysicsCubeS");

    Ref<Scene> scene = SceneManager::Get().NewScene();

    Entity env = scene->AddEntity("Env");
    EnvironmentComponent& envComp = scene->AddComponent<EnvironmentComponent>(env);
    envComp.settings.ambient = Color{0.11f, 0.16f, 0.25f, 1};
    envComp.settings.shadowDistance = 1000;

    Entity light = scene->AddEntity("Light");
    LightComponent& lightComponent = scene->AddComponent<LightComponent>(light);
    lightComponent.color = {1,1,1};
    scene->GetComponent<TransformComponent>(light).Position(Vector3(-2, 4, -1));
    scene->GetComponent<TransformComponent>(light).LocalEulerAngles(Vector3(45, -125, 0));
    lightComponent.renderShadow = true;

    camera = scene->AddEntity("Camera");
    CameraComponent& cam = scene->AddComponent<CameraComponent>(camera);
    scene->GetComponent<TransformComponent>(camera).LocalPosition(Vector3(-37.4206, 0, 38.0931));
    scene->GetComponent<TransformComponent>(camera).LocalEulerAngles(Vector3(0, 0, 0));
    scene->AddComponent<ScriptComponent>(camera).AddScript<CameraMovementScript>()->moveSpeed = 160;
    cam.farClipPlane = 10000;
    cam.fieldOfView = 60;

    Ref<Model> cubeModel = AssetManager::Get().LoadAsset<Model>("Sandbox/Models/Cube.glb");
    cubeModel->SetShader(AssetManager::Get().LoadAsset<Shader>("Engine/Shaders/Lit.glsl"));

    Entity cube = scene->AddEntity("Cube");
    TransformComponent& cubeTrans = scene->GetComponent<TransformComponent>(cube);
    cubeTrans.LocalScale(Vector3(50, 200, 50));
    cubeTrans.LocalPosition(Vector3(256, 100, -256));
    ModelRendererComponent& cubeModelRenderer = scene->AddComponent<ModelRendererComponent>(cube);
    cubeModelRenderer.SetModel(cubeModel);

    Entity terrain = scene->AddEntity("Terrain");
    TerrainComponent& terrainComponent = scene->AddComponent<TerrainComponent>(terrain);
    //terrainComponent.terrainWidth = 1000;
    //terrainComponent.terrainLength = 500;
    terrainComponent.terrainHeight = 420;
    int heightmapSize = (1024 * 1)+1;
    terrainComponent.SetHeightmap(
        GenerateHeightmap(heightmapSize, heightmapSize, 50, 0.25f/(4*1), 4, 0.5f, 2.0f, Vector2(0, 0))
    );
    terrainComponent.GetHeightmap()->Save("Sandbox/Datas/Terrain.heightmap", Asset::SaveType::AssetBinary);
    terrainComponent.splatmap = AssetManager::Get().LoadAsset<Texture2D>("Sandbox/Textures/rgb-splat-map.png");
    terrainComponent.layer0 = AssetManager::Get().LoadAsset<Texture2D>("Sandbox/Textures/block.png");
    terrainComponent.layer1 = AssetManager::Get().LoadAsset<Texture2D>("Sandbox/Textures/brickwall.jpg");
    terrainComponent.layer2 = AssetManager::Get().LoadAsset<Texture2D>("Sandbox/Textures/floor.jpg");
    terrainComponent.layer3 = AssetManager::Get().LoadAsset<Texture2D>("Sandbox/Textures/Rock.jpg");
    terrainComponent.layer4 = AssetManager::Get().LoadAsset<Texture2D>("Sandbox/Textures/block.png");

    /*Entity navmesh = scene->AddEntity("Navmesh");
    auto& nav = scene->AddComponent<NavmeshComponent>(navmesh);
    nav.navmesh = CreateRef<Navmesh>();
    nav.navmesh->Bake(scene, AABB(Vector3(0), 1000, 1000, 1000), {});*/ 

    Application::AddModule<Editor>();
    //scene->Start();
}

void TerrainSample::OnUpdate(float deltaTime){
    if(Input::IsKeyDown(KeyCode::T)) RenderContext::GetSettings().enableWireframe = !RenderContext::GetSettings().enableWireframe;
    if(Input::IsKeyDown(KeyCode::Y)) RenderContext::GetSettings().enableGizmosRuntime = !RenderContext::GetSettings().enableGizmosRuntime;

    Ref<Scene> scene = SceneManager::Get().GetActiveScene();
    if(scene->Running() == false) return;

    if(Input::IsKeyDown(KeyCode::R)){
        for(int i = 0; i < 5; i++){
        Entity e = SceneManager::Get().GetActiveScene()->AddEntity("PhysicsCube");
        TransformComponent& camTrans = scene->GetComponent<TransformComponent>(scene->GetMainCamera());

        scene->GetComponent<TransformComponent>(e).Position(camTrans.Position() + camTrans.Back() * 2.0f);
        scene->GetComponent<TransformComponent>(e).Rotation(QuaternionIdentity);
        scene->AddComponent<ScriptComponent>(e).AddScript<PhysicsCubeS>()->timeToDestroy = 100000000;
        }
    }

    if(Input::IsKeyDown(KeyCode::T)){
        Entity terrain = scene->FindEntityByName("Terrain");
        TerrainComponent& terr = scene->GetComponent<TerrainComponent>(terrain);
        terr.GetHeightmap()->Save("Sandbox/Datas/Terrain.heightmap",  Asset::SaveType::AssetBinary);
    }

    if(Input::IsKeyDown(KeyCode::Y)){
        Entity terrain = scene->FindEntityByName("Terrain");
        Ref<Heightmap> heightmap = CreateRef<Heightmap>();
        heightmap->LoadFromFile("Sandbox/Datas/Terrain.heightmap");

        TerrainComponent& terr = scene->GetComponent<TerrainComponent>(terrain);
        terr.SetHeightmap(heightmap);
    }
}   

void TerrainSample::OnRender(float deltaTime){}
void TerrainSample::OnGUI(){}
void TerrainSample::OnResize(int width, int height){}
void TerrainSample::OnExit(){}