#include "ProceduralTerrain2.h"
#include "Ultis/CameraMovement.h"
#include <assert.h>
#include "Ultis/Ultis.h"
#include "Ultis/FastNoiseLiteCpp.h"

struct ObjectsBuck{
    std::string id;
    std::vector<Ref<Model>> models;
};

struct ObjectsRef{
    int objectsIndex;
    Vector3 pos;
    Vector3 euler;
    Vector3 scale;
};

struct AreaSpawnSettings{
    ObjectsBuck& objectsBuck;
    int count;
    Vector2 miMaxZRange;
    Vector2 miMaxXRange;
    
    Vector2 minMaxZAngle;

    Vector3 minScale = {1, 1, 1};
    Vector3 maxScale = {5*5, 20*5, 5*5};
};

inline void GenerateByArea(const AreaSpawnSettings& areaSpawnSettings, const Ref<Heightmap> heightmap, float heightScale, std::vector<ObjectsRef>& out){
    out.clear();
    for(int i = 0; i < areaSpawnSettings.count; i++){
        /*Vector3 targetPos(
            random(areaSpawnSettings.miMaxXRange.x, areaSpawnSettings.miMaxXRange.y),
            0,
            random(areaSpawnSettings.minMaxZAngle.x, areaSpawnSettings.minMaxZAngle.y)
        );*/

        int tx = random(0, heightmap->width-1);
        int ty = random(0, heightmap->height-1);
        Vector3 targetPos(
            tx,
            heightmap->Get(tx, ty) * heightScale,
            -ty
        );
        Vector3 targetEulerZ(
            0,
            random(areaSpawnSettings.minMaxZAngle.x, areaSpawnSettings.minMaxZAngle.y),
            0
        );
        Vector3 targetScale(
            random(areaSpawnSettings.minScale.x, areaSpawnSettings.maxScale.x),
            random(areaSpawnSettings.minScale.y, areaSpawnSettings.maxScale.y),
            random(areaSpawnSettings.minScale.z, areaSpawnSettings.maxScale.z)
        );

        out.push_back({
            random(0, areaSpawnSettings.objectsBuck.models.size()-1),
            targetPos,
            targetEulerZ,
            targetScale
        });
    }
}

inline void SpawnObjectsRef(/*PhysicsSystem& physicsSystem,*/ Scene& scene, Entity& root, Entity terrain, ObjectsBuck& objectsBuck, std::vector<ObjectsRef>& out){
    for(ObjectsRef& or: out){
        Entity e = scene.AddEntity();
        scene.SetParent(root,  e);

        TransformComponent& terrainTrans = scene.GetComponent<TransformComponent>(terrain);
        TransformComponent& trans = scene.GetComponent<TransformComponent>(e);
        float scale = 8000.0f / ((1024.0f*4.0f)+1.0f);
        trans.Position(terrainTrans.TransformPoint(or.pos * Vector3(scale, 1, scale)));
        //trans.LocalPosition(or.pos);
        trans.LocalEulerAngles(or.euler);
        //trans.LocalScale(Vector3One * 25.0f);
        trans.LocalScale(or.scale);
        
        /*RayResult hit;
        if(physicsSystem.Raycast(trans.Position() + Vector3(0, 2000, 0), Vector3Down * 10000.0f, hit)){
            trans.Position(hit.hitPoint);
        }*/

        auto& modelRenderer = scene.AddComponent<StaticModelRendererComponent>(e);
        modelRenderer.SetModel(objectsBuck.models[or.objectsIndex]);
    }
}

Ref<Heightmap> ProceduralTerrain2::GenerateHeightmap(int mapWidth, int mapHeight, int seed, float scale, int octaves, float persistance, float lacunarity, Vector2 offset){
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

void ProceduralTerrain2::OnInit(){
    LogInfo("%sGame Init %s", "\033[0;32m", "\033[0m");
    Application::Vsync(false);
    SceneManager::Get().RegisterScript<CameraMovementScript>("CameraMovementScript");

    Scene* scene = SceneManager::Get().NewScene();

    Entity env = scene->AddEntity("Env");
    EnvironmentComponent& envComp = scene->AddComponent<EnvironmentComponent>(env);
    envComp.settings.ambient = Color{0.11f, 0.16f, 0.25f, 1};
    envComp.settings.shadowDistance = 5000;

    Entity light = scene->AddEntity("Light");
    LightComponent& lightComponent = scene->AddComponent<LightComponent>(light);
    lightComponent.color = {1,1,1};
    scene->GetComponent<TransformComponent>(light).Position(Vector3(-2, 4, -1));
    scene->GetComponent<TransformComponent>(light).LocalEulerAngles(Vector3(45, -125, 0));
    lightComponent.renderShadow = true;

    camera = scene->AddEntity("Camera");
    CameraComponent& cam = scene->AddComponent<CameraComponent>(camera);
    scene->GetComponent<TransformComponent>(camera).LocalPosition(Vector3(-37.4206, 2000, 38.0931));
    scene->GetComponent<TransformComponent>(camera).LocalEulerAngles(Vector3(-70.2000, -11.4000, 0));
    scene->AddComponent<ScriptComponent>(camera).AddScript<CameraMovementScript>()->moveSpeed = 350;
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

    terrain = scene->AddEntity("Terrain");
    TransformComponent& terrainTrans = scene->GetComponent<TransformComponent>(terrain);
    terrainTrans.LocalPosition({-5000, 0, 5000});
    TerrainComponent& terrainComponent = scene->AddComponent<TerrainComponent>(terrain);
    terrainComponent.terrainWidth = 8000;
    terrainComponent.terrainLength = 8000;
    terrainComponent.terrainHeight = 600;
    terrainComponent.chunkWidthCount = 10;
    terrainComponent.mapChunkSize = (512 * 1) + 1;
    terrainComponent.lodBias = 1;
    
    heightmapSize = (1024 * 4)+1;
    seed = 50;
    scale = 0.25f/(4*1);
    octaves = 4;
    persistance = 0.5f;
    lacunarity = 2.0f;
    offset = Vector2(0, 0);

    Ref<Heightmap> heighmap = nullptr;
    {
    OD_LOG_PROFILE("ProceduralTerrain2::GenerateHeightmap");
    heighmap = GenerateHeightmap(heightmapSize, heightmapSize, seed, scale, octaves, persistance, lacunarity, offset);
    terrainComponent.SetHeightmap(
        //GenerateHeightmap(heightmapSize, heightmapSize, 50, 0.25f/(4*1), 4, 0.5f, 2.0f, Vector2(0, 0))
        heighmap
    );
    }
    terrainComponent.splatmap = AssetManager::Get().LoadAsset<Texture2D>("Sandbox/Textures/rgb-splat-map.png");
    terrainComponent.layer0 = AssetManager::Get().LoadAsset<Texture2D>("Sandbox/Textures/block.png");
    terrainComponent.layer1 = AssetManager::Get().LoadAsset<Texture2D>("Sandbox/Textures/brickwall.jpg");
    terrainComponent.layer2 = AssetManager::Get().LoadAsset<Texture2D>("Sandbox/Textures/floor.jpg");
    terrainComponent.layer3 = AssetManager::Get().LoadAsset<Texture2D>("Sandbox/Textures/Rock.jpg");
    terrainComponent.layer4 = AssetManager::Get().LoadAsset<Texture2D>("Sandbox/Textures/block.png");

    std::unordered_map<std::string, ObjectsBuck> objectsBucks;
    objectsBucks["Rocks"] = {
        "Rock",
        std::vector<Ref<Model>>{
            AssetManager::Get().LoadAsset<Model>("Sandbox/Models/Cube.glb"),
            //AssetManager::Get().LoadAsset<Model>("Game/TempModels/kenney_city-kit/Models/GLTF format/large_buildingA.glb"),
            //AssetManager::Get().LoadAsset<Model>("Game/TempModels/kenney_city-kit/Models/GLTF format/large_buildingB.glb"),
            //AssetManager::Get().LoadAsset<Model>("Game/TempModels/kenney_city-kit/Models/GLTF format/large_buildingC.glb"),
            //AssetManager::Get().LoadAsset<Model>("Game/TempModels/kenney_city-kit/Models/GLTF format/large_buildingD.glb"),
            //AssetManager::Get().LoadAsset<Model>("Game/TempModels/kenney_city-kit/Models/GLTF format/large_buildingE.glb"),
        }
    };
    for(auto& i: objectsBucks["Rocks"].models){
        for(auto& j: i->materials){
            j->SetShader(AssetManager::Get().LoadAsset<Shader>("Engine/Shaders/Lit.glsl"));
            j->SetEnableInstancing(true);
        }
    }

    Entity spawnObjects = scene->AddEntity("SpawnObjects");

    AreaSpawnSettings spawnSettings{
        objectsBucks["Rocks"],
        30000*2
    };
    spawnSettings.maxScale = {25, 50, 25};
    std::vector<ObjectsRef> toSpawn;
    GenerateByArea(spawnSettings, heighmap, terrainComponent.terrainHeight, toSpawn);
    SpawnObjectsRef(*scene, spawnObjects, terrain, objectsBucks["Rocks"], toSpawn);

    Application::AddModule<Editor>();
    //scene->Start();
}

void ProceduralTerrain2::OnUpdate(float deltaTime){
    if(Input::IsKeyDown(KeyCode::T)) RenderContext::GetSettings().enableWireframe = !RenderContext::GetSettings().enableWireframe;
    if(Input::IsKeyDown(KeyCode::Y)) RenderContext::GetSettings().enableGizmosRuntime = !RenderContext::GetSettings().enableGizmosRuntime;
}   

void ProceduralTerrain2::OnRender(float deltaTime){}

void ProceduralTerrain2::OnGUI(){
    Scene* scene = SceneManager::Get().GetActiveScene();

    ImGui::Begin("Terrain Settings");
    ImGui::DragInt("seed", &seed);
    ImGui::DragFloat("scale", &scale);
    ImGui::DragInt("octaves", &octaves);
    ImGui::DragFloat("persistance", &persistance);
    ImGui::DragFloat("lacunarity", &lacunarity);
    ImGui::DragFloat2("offset", &offset.x);
    if(ImGui::Button("Update Heightmap")){
        TerrainComponent& t = scene->GetComponent<TerrainComponent>(terrain);
        t.SetHeightmap(
            GenerateHeightmap(heightmapSize, heightmapSize, seed, scale, octaves, persistance, lacunarity, offset)
        );
    }
    ImGui::End();
}

void ProceduralTerrain2::OnResize(int width, int height){}
void ProceduralTerrain2::OnExit(){}