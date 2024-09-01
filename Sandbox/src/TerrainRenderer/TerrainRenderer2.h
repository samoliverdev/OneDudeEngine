#pragma once
#include <OD/OD.h>
#include "../Ultis/CameraMovement.h"
#include <assert.h>
#include "Ultis/Ultis.h"
#include "ProceduralTerrain/Noise.h"

using namespace OD;

struct TerrainRenderer2: OD::Module {
    Entity camera;
    Entity terrain;
    Ref<Material> terrainMaterial;
    
    void OnInit() override {
        Application::Vsync(false);

        LogInfo("%sGame Init %s", "\033[0;32m", "\033[0m");

        SceneManager::Get().RegisterScript<CameraMovementScript>("CameraMovementScript");

        Scene* scene = SceneManager::Get().NewScene();

        Entity text = scene->AddEntity("Text");
        text.GetComponent<TransformComponent>().LocalPosition(Vector3(25.0f, 25.0f, 0));
        TextRendererComponent& textRenderer = text.AddComponent<TextRendererComponent>();
        textRenderer.text = "ProceduralTerrain";
        textRenderer.color = {0.5f, 0.8f, 0.2f, 1.0f};
        textRenderer.font = CreateRef<Font>("res/Engine/Fonts/OpenSans/static/OpenSans_Condensed-Bold.ttf");
        textRenderer.material = CreateRef<Material>(Shader::CreateFromFile("res/Engine/Shaders/Font.glsl"));

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

        Ref<Model> clipmapMesh = AssetManager::Get().LoadAsset<Model>("res/Game/Models/TerrainTesselationMesh.glb");
        
        /*Ref<Texture2D> heightMap = AssetManager::Get().LoadAsset<Texture2D>(
            //"res/Game/Textures/043-ue4-heightmap-guide-02.jpg", 
            "res/Game/Textures/heightmap-2.jpg",
            Texture2DSetting{TextureFilter::Linear, TextureWrapping::ClampToEdge, true}
        );*/

        Ref<NoiseData> dataTest = Noise::GenerateNoiseMap(512, 512, 50, 0.25f, 4, 1, 1, Vector2(0, 0));
        Ref<Texture2D> heightMap = CreateRef<Texture2D>(
            (void*)&dataTest->data[0],
            (size_t)(dataTest->data.size() * sizeof(float)),
            512, 512,
            TextureDataType::Float,
            Texture2DSetting{TextureFilter::Linear, TextureWrapping::ClampToEdge, true, TextureFormat::RED16F}
        );

        Ref<Shader> terrainShader = AssetManager::Get().LoadAsset<Shader>("res/Game/Shaders/TerrainClipmapMesh.glsl");
        terrainMaterial = CreateRef<Material>(terrainShader);
        terrainMaterial->SetTexture("mainTex", AssetManager::Get().LoadAsset<Texture2D>("res/Game/Textures/floor.jpg"));
        terrainMaterial->SetTexture("heightMap", heightMap);
        terrainMaterial->SetVector4("color", Vector4(1, 1, 1, 1));
        terrainMaterial->SetFloat("heightScale", 25*0.25f);
        
        Assert(clipmapMesh->meshs[0]->tangents.size() > 0);

        terrain = scene->AddEntity("Terrain");
        terrain.GetComponent<TransformComponent>().LocalScale(Vector3(2));
        //terrain.GetComponent<TransformComponent>().LocalEulerAngles(Vector3(0, 90, 0));
        MeshRendererComponent& terrainMeshRenderer = terrain.AddComponent<MeshRendererComponent>();
        terrainMeshRenderer.mesh = clipmapMesh->meshs[0];
        terrainMeshRenderer.UpdateAABB();
        terrainMeshRenderer.material = terrainMaterial;

        scene->Start();
        Application::AddModule<Editor>();
    }

    void OnUpdate(float deltaTime) override {
        Scene* scene = SceneManager::Get().GetActiveScene();
        if(scene->Running() == false) return;

        if(Input::IsKeyDown(KeyCode::T)) RenderContext::GetSettings().enableGizmosRuntime = !RenderContext::GetSettings().enableGizmosRuntime;
 
        if(Input::IsKeyDown(KeyCode::R)){
            TransformComponent& cam = scene->GetMainCamera2().GetComponent<TransformComponent>();

            Entity e = SceneManager::Get().GetActiveScene()->AddEntity("PhysicsCube");
            e.GetComponent<TransformComponent>().Position(cam.Position() + cam.Back() * 20.0f);
            e.GetComponent<TransformComponent>().LocalScale(Vector3(2));
            PhysicsCubeS* script = e.AddComponent<ScriptComponent>().AddScript<PhysicsCubeS>();
            script->timeToDestroy = 1000000;
        }
    } 

    void OnRender(float deltaTime) override {}
    
    void OnGUI() override {
        /*ImGui::Begin("Test Icon");

        ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(0,255,0,255));
        ImGui::Text(ICON_FA_PAINT_BRUSH "  Paint" );
        ImGui::PopStyleColor();
        
        ImGui::Text("%s  among %d items", ICON_FA_SEARCH, 10);

        ImGui::Text(ICON_FA_PAINT_BRUSH "  "); // use string literal concatenation
        ImGui::SameLine(); 
        ImGui::Button("Search");
        
        ImGui::Button(ICON_FA_THUMBS_UP "  Search");
        
        ImGui::End();*/
    }

    void OnResize(int width, int height) override {}
    void OnExit() override {}
};