#pragma once
#include <OD/OD.h>
#include "../Ultis/CameraMovement.h"
#include <assert.h>
#include "Ultis/Ultis.h"
#include "ProceduralTerrain/Noise.h"

using namespace OD;

struct TerrainRenderer3: OD::Module {
    Entity camera;
    Entity terrain;
    Ref<Material> terrainMaterial;
    
    void OnInit() override {
        LogInfo("%sGame Init %s", "\033[0;32m", "\033[0m");

        SceneManager::Get().RegisterScript<CameraMovementScript>("CameraMovementScript");

        Scene* scene = SceneManager::Get().NewScene();

        Entity text = scene->AddEntity("Text");
        text.GetComponent<TransformComponent>().LocalPosition(Vector3(25.0f, 25.0f, 0));
        TextRendererComponent& textRenderer = text.AddComponent<TextRendererComponent>();
        textRenderer.text = "ProceduralTerrain";
        textRenderer.color = {0.5f, 0.8f, 0.2f, 1.0f};
        textRenderer.font = Font::CreateFromFile("Engine/Fonts/OpenSans/static/OpenSans_Condensed-Bold.ttf");
        textRenderer.material = CreateRef<Material>(Shader::CreateFromFile("Engine/Shaders/Font.glsl"));

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
        camera.GetComponent<TransformComponent>().LocalPosition(Vector3(0, 10, 15));
        camera.GetComponent<TransformComponent>().LocalEulerAngles(Vector3(-25, 0, 0));
        camera.AddComponent<ScriptComponent>().AddScript<CameraMovementScript>()->moveSpeed = 60*4;
        cam.farClipPlane = 50000;

        Ref<Model> clipmapMesh = AssetManager::Get().LoadAsset<Model>("Sandbox/Models/TerrainPlane.glb");
        
        /*Ref<Texture2D> heightMap = AssetManager::Get().LoadAsset<Texture2D>(
            //"res/Game/Textures/043-ue4-heightmap-guide-02.jpg", 
            "Sandbox/Textures/heightmap-2.jpg",
            Texture2DSetting{TextureFilter::Linear, TextureWrapping::ClampToEdge, true}
        );*/

        int _size = 512;

        Ref<NoiseData> dataTest = Noise::GenerateNoiseMap(_size, _size, 50, 0.5f, 4, 1, 1, Vector2(0, 0));
        Ref<Texture2D> heightMap = Texture2D::CreateFromRaw( //CreateRef<Texture2D>(
            (void*)&dataTest->data[0],
            (size_t)(dataTest->data.size() * sizeof(float)),
            _size, _size,
            TextureDataType::Float,
            Texture2DSetting{TextureFilter::Linear, TextureWrapping::ClampToEdge, true, TextureFormat::RED16F}
        );

        Ref<Shader> terrainShader = AssetManager::Get().LoadAsset<Shader>("Sandbox/Shaders/TerrainClipmapMesh.glsl");
        terrainMaterial = CreateRef<Material>(terrainShader);
        terrainMaterial->SetTexture("mainTex", AssetManager::Get().LoadAsset<Texture2D>("Sandbox/Textures/floor.jpg"));
        terrainMaterial->SetTexture("heightMap", heightMap);
        terrainMaterial->SetVector4("color", Vector4(1, 1, 1, 1));
        terrainMaterial->SetFloat("heightScale", 10);
        
        Assert(clipmapMesh->meshs[0]->tangents.size() > 0);

        terrain = scene->AddEntity("Terrain");
        terrain.GetComponent<TransformComponent>().LocalPosition(Vector3(0, -5, 0));
        terrain.GetComponent<TransformComponent>().LocalScale(Vector3(_size, 1, _size));
        terrain.GetComponent<TransformComponent>().LocalEulerAngles(Vector3(0, 0, 0));
        MeshRendererComponent& terrainMeshRenderer = terrain.AddComponent<MeshRendererComponent>();
        terrainMeshRenderer.mesh = clipmapMesh->meshs[0];
        terrainMeshRenderer.UpdateAABB();
        terrainMeshRenderer.material = terrainMaterial;
        
        HeightmapColliderComponent& heightmapCollider = terrain.AddComponent<HeightmapColliderComponent>();
        heightmapCollider.width = _size;
        heightmapCollider.length = _size;
        heightmapCollider.scale = _size;
        heightmapCollider.minHeight = 0;
        heightmapCollider.maxHeight = 10;

        for(int i = 0; i < dataTest->data.size(); i++){
            heightmapCollider.heights.push_back(dataTest->data[i] * 10);
        }

        /*heightmapCollider.heights.resize(_size*_size);
        for(int i = 0, j = dataTest->data.size() - _size; i < dataTest->data.size(); i += _size, j -= _size) {
            for(int k = 0; k < _size; ++k) {
                heightmapCollider.heights[i + k] = dataTest->data[j + k] * 10;
                Assert(heightmapCollider.heights[i + k] >= 0);
                Assert(heightmapCollider.heights[i + k] <= 10);
                //if(heightmapCollider.heights[i + k] < 0) heightmapCollider.heights[i + k] = 0;
            }
        }*/

        RenderContext::GetSettings().enableGizmosRuntime = true;
        //scene->Start();
        Application::AddModule<Editor>();
    }

    void OnUpdate(float deltaTime) override {
        Scene* scene = SceneManager::Get().GetActiveScene();
        if(scene->Running() == false) return;

        float snapStep = 100;
        int div = 256*100;

        Vector3 camPos = scene->GetMainCamera().GetComponent<TransformComponent>().Position();
        Vector3 snapCamera = Vector3(math::round(camPos.x * (1 / snapStep))*snapStep, 0, math::round(camPos.z * (1/snapStep))*snapStep);
        //Vector3 snapCamera = Vector3(camPos.x, 0, camPos.z);

        //terrain.GetComponent<TransformComponent>().Position(snapCamera);
        //terrainMaterial->SetVector2("uvOffset", Vector2((snapCamera.x/div), -(snapCamera.z/div)));      

        if(Input::IsKeyDown(KeyCode::R)){
            TransformComponent& cam = scene->GetMainCamera().GetComponent<TransformComponent>();

            Entity e = SceneManager::Get().GetActiveScene()->AddEntity("PhysicsCube");
            e.GetComponent<TransformComponent>().Position(cam.Position() + cam.Back() * 10.0f);
            PhysicsCubeS* script = e.AddComponent<ScriptComponent>().AddScript<PhysicsCubeS>();
            script->timeToDestroy = 1000000;
        }

        if(Input::IsKeyDown(KeyCode::T)){
            RenderContext::GetSettings().enableGizmosRuntime = !RenderContext::GetSettings().enableGizmosRuntime;
        }
    } 

    void OnRender(float deltaTime) override {}
    void OnGUI() override {}

    void OnResize(int width, int height) override {}
    void OnExit() override {}
};