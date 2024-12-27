#pragma once
#include <OD/OD.h>
#include "../Ultis/CameraMovement.h"
#include <assert.h>
#include "Ultis/Ultis.h"
#include "ProceduralTerrain/Noise.h"

using namespace OD;

struct TerrainRenderer: OD::Module {
    Entity camera;
    Entity terrain;
    Ref<Material> terrainMaterial;
    
    void OnInit() override {
        LogInfo("%sGame Init %s", "\033[0;32m", "\033[0m");

        SceneManager::Get().RegisterScript<CameraMovementScript>("CameraMovementScript");

        Scene* scene = SceneManager::Get().NewScene();

        Entity text = scene->AddEntity("Text");
        scene->GetComponent<TransformComponent>(text).LocalPosition(Vector3(25.0f, 25.0f, 0));
        TextRendererComponent& textRenderer = scene->AddComponent<TextRendererComponent>(text);
        textRenderer.text = "ProceduralTerrain";
        textRenderer.color = {0.5f, 0.8f, 0.2f, 1.0f};
        textRenderer.font = Font::CreateFromFile("res/Engine/Fonts/OpenSans/static/OpenSans_Condensed-Bold.ttf");
        textRenderer.material = CreateRef<Material>(Shader::CreateFromFile("res/Engine/Shaders/Font.glsl"));

        Entity env = scene->AddEntity("Env");
        scene->AddComponent<EnvironmentComponent>(env).settings.ambient = Color{0.11f, 0.16f, 0.25f, 1};

        Entity light = scene->AddEntity("Light");
        LightComponent& lightComponent = scene->AddComponent<LightComponent>(light);
        lightComponent.color = {1,1,1};
        scene->GetComponent<TransformComponent>(light).Position(Vector3(-2, 4, -1));
        scene->GetComponent<TransformComponent>(light).LocalEulerAngles(Vector3(45, -125, 0));
        lightComponent.renderShadow = false;

        camera = scene->AddEntity("Camera");
        CameraComponent& cam = scene->AddComponent<CameraComponent>(camera);
        scene->GetComponent<TransformComponent>(camera).LocalPosition(Vector3(0, 1000, 15));
        scene->GetComponent<TransformComponent>(camera).LocalEulerAngles(Vector3(-25, 0, 0));
        scene->AddComponent<ScriptComponent>(camera).AddScript<CameraMovementScript>()->moveSpeed = 60*4;
        cam.farClipPlane = 50000;

        Ref<Model> clipmapMesh = AssetManager::Get().LoadAsset<Model>("res/Game/Models/ClipmapMesh_High1.glb");
        
        Ref<Texture2D> heightMap = Texture2D::CreateFromFile( //AssetManager::Get().LoadAsset<Texture2D>(
            //"res/Game/Textures/043-ue4-heightmap-guide-02.jpg", 
            "res/Game/Textures/heightmap-2.jpg",
            Texture2DSetting{TextureFilter::Linear, TextureWrapping::ClampToEdge, true}
        );

        /*Ref<NoiseData> dataTest = Noise::GenerateNoiseMap(512, 512, 50, 0.5f, 4, 1, 1, Vector2(0, 0));
        Ref<Texture2D> heightMap = CreateRef<Texture2D>(
            (void*)&dataTest->data[0],
            (size_t)(dataTest->data.size() * sizeof(float)),
            512, 512,
            TextureDataType::Float,
            Texture2DSetting{TextureFilter::Linear, TextureWrapping::ClampToEdge, true, TextureFormat::RED16F}
        );*/

        Ref<Shader> terrainShader = AssetManager::Get().LoadAsset<Shader>("res/Game/Shaders/TerrainClipmapMesh.glsl");
        terrainMaterial = CreateRef<Material>(terrainShader);
        terrainMaterial->SetTexture("mainTex", AssetManager::Get().LoadAsset<Texture2D>("res/Game/Textures/floor.jpg"));
        terrainMaterial->SetTexture("heightMap", heightMap);
        terrainMaterial->SetVector4("color", Vector4(1, 1, 1, 1));
        terrainMaterial->SetFloat("heightScale", 25*1.0f);
        
        Assert(clipmapMesh->meshs[0]->tangents.size() > 0);

        terrain = scene->AddEntity("Terrain");
        scene->GetComponent<TransformComponent>(terrain).LocalScale(Vector3(100));
        scene->GetComponent<TransformComponent>(terrain).LocalEulerAngles(Vector3(0, 90, 0));
        MeshRendererComponent& terrainMeshRenderer = scene->AddComponent<MeshRendererComponent>(terrain);
        terrainMeshRenderer.mesh = clipmapMesh->meshs[0];
        terrainMeshRenderer.UpdateAABB();
        terrainMeshRenderer.material = terrainMaterial;
        /*HeightmapColliderComponent& heightmapCollider = terrain.AddComponent<HeightmapColliderComponent>();
        heightmapCollider.width = 256*100;
        heightmapCollider.length = 256*100;
        heightmapCollider.scale = 512;
        heightmapCollider.minHeight = 0;
        heightmapCollider.maxHeight = (25*0.25f)*100;
        for(int i = 0; i < dataTest->data.size(); i++){
            heightmapCollider.heights.push_back(dataTest->data[i] * (25*0.25f)*100);
        }*/

        RenderContext::GetSettings().enableGizmosRuntime = true;
        scene->Start();
        //Application::AddModule<Editor>();
    }

    void OnUpdate(float deltaTime) override {
        Scene* scene = SceneManager::Get().GetActiveScene();
        if(scene->Running() == false) return;

        float snapStep = 100;
        int div = 256*100;

        Vector3 camPos = scene->GetComponent<TransformComponent>(scene->GetMainCamera()).Position();
        Vector3 snapCamera = Vector3(math::round(camPos.x * (1 / snapStep))*snapStep, 0, math::round(camPos.z * (1/snapStep))*snapStep);
        //Vector3 snapCamera = Vector3(camPos.x, 0, camPos.z);

        scene->GetComponent<TransformComponent>(terrain).Position(snapCamera);
        terrainMaterial->SetVector2("uvOffset", Vector2((snapCamera.x/div), -(snapCamera.z/div)));      

        if(Input::IsKeyDown(KeyCode::R)){
            TransformComponent& cam = scene->GetComponent<TransformComponent>(scene->GetMainCamera());

            Entity e = SceneManager::Get().GetActiveScene()->AddEntity("PhysicsCube");
            scene->GetComponent<TransformComponent>(e).Position(cam.Position() + cam.Back() * 10.0f);
            PhysicsCubeS* script = scene->AddComponent<ScriptComponent>(e).AddScript<PhysicsCubeS>();
            script->timeToDestroy = 1000000;
        }
    } 

    void OnRender(float deltaTime) override {}
    void OnGUI() override {}

    void OnResize(int width, int height) override {}
    void OnExit() override {}
};