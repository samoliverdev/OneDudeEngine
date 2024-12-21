#pragma once
#include <OD/OD.h>
#include "../Ultis/CameraMovement.h"
#include <assert.h>
#include "Ultis/Ultis.h"
#include "Terrain1.h"
#include "Terrain2.h"

using namespace OD;

struct TerrainRenderer4: OD::Module {
    Entity camera;
    Entity terrain;
    Ref<Material> terrainMaterial;
    
    void OnInit() override {
        LogInfo("%sGame Init %s", "\033[0;32m", "\033[0m");

        Application::Vsync(false);
        SceneManager::Get().RegisterScript<CameraMovementScript>("CameraMovementScript");
        SceneManager::Get().RegisterScript<Terrain1>("Terrain1");
        SceneManager::Get().RegisterScript<Terrain2>("Terrain2");
        Scene* scene = SceneManager::Get().NewScene();

        Entity env = scene->AddEntity("Env");
        env.AddComponent<EnvironmentComponent>().settings.ambient = Color{0.11f, 0.16f, 0.25f, 1};

        Entity light = scene->AddEntity("Light");
        LightComponent& lightComponent = light.AddComponent<LightComponent>();
        lightComponent.color = {1,1,1};
        light.GetComponent<TransformComponent>().Position(Vector3(-2, 4, -1));
        light.GetComponent<TransformComponent>().LocalEulerAngles(Vector3(45, -125, 0));
        lightComponent.renderShadow = false;

        camera = scene->AddEntity("Camera");
        CameraComponent& cam = camera.AddComponent<CameraComponent>();
        camera.GetComponent<TransformComponent>().LocalPosition(Vector3(0, 100, 15));
        camera.GetComponent<TransformComponent>().LocalEulerAngles(Vector3(-25, 0, 0));
        camera.AddComponent<ScriptComponent>().AddScript<CameraMovementScript>()->moveSpeed = 60*2;
        cam.farClipPlane = 50000;

        Ref<Model> clipmapMesh = AssetManager::Get().LoadAsset<Model>("Sandbox/Models/TerrainTesselationMesh.glb");
        
        /*Ref<Texture2D> heightMap = AssetManager::Get().LoadAsset<Texture2D>(
            //"res/Game/Textures/043-ue4-heightmap-guide-02.jpg", 
            "res/Game/Textures/heightmap-2.jpg",
            Texture2DSetting{TextureFilter::Linear, TextureWrapping::ClampToEdge, true}
        );*/

        Ref<NoiseData> dataTest = Noise::GenerateNoiseMap(512, 512, 50, 0.25f, 4, 1, 1, Vector2(0, 0));
        Ref<Texture2D> heightMap = Texture2D::CreateFromRaw( //CreateRef<Texture2D>(
            (void*)&dataTest->data[0],
            (size_t)(dataTest->data.size() * sizeof(float)),
            512, 512,
            TextureDataType::Float,
            Texture2DSetting{TextureFilter::Linear, TextureWrapping::ClampToEdge, true, TextureFormat::RED16F}
        );

        terrainMaterial = CreateRef<Material>(
            AssetManager::Get().LoadAsset<Shader>("Sandbox/Shaders/TerrainClipmapMesh.glsl")
        );
        terrainMaterial->SetTexture("mainTex", AssetManager::Get().LoadAsset<Texture2D>("Sandbox/Textures/floor.jpg"));
        terrainMaterial->SetTexture("heightMap", heightMap);
        terrainMaterial->SetVector4("color", Vector4(1, 1, 1, 1));
        terrainMaterial->SetFloat("heightScale", 25*0.25f);
        
        Assert(clipmapMesh->meshs[0]->tangents.size() > 0);

        /*terrain = scene->AddEntity("Terrain");
        //terrain.GetComponent<TransformComponent>().LocalScale(Vector3(256, 1, 256));
        //terrain.GetComponent<TransformComponent>().LocalEulerAngles(Vector3(0, 90, 0));
        MeshRendererComponent& terrainMeshRenderer = terrain.AddComponent<MeshRendererComponent>();
        terrainMeshRenderer.mesh = GenerateTerrainMesh1(1024+1, 1024+1, 0, {true, false, false, true}); //clipmapMesh->meshs[0];
        terrainMeshRenderer.UpdateAABB();
        terrainMeshRenderer.material = terrainMaterial;*/

        terrain = scene->AddEntity("Terrain");
        auto* terrainScript = terrain.AddComponent<ScriptComponent>().AddScript<Terrain1>();
        //terrainScript->viewer = &camera.GetComponent<TransformComponent>();

        scene->Start();
        Application::AddModule<Editor>();
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