#include "LoadModel.h"
#include "Ultis/Ultis.h"

void LoadModelSample::OnInit(){
    LogInfo("Game Init");

    Application::Vsync(false);

    camTransform.LocalPosition(Vector3(0, 10, 70));
    camTransform.LocalEulerAngles(Vector3(0, 0, 0));
    camMove.transform = &camTransform;

    model = AssetManager::Get().LoadAsset<Model>("Sandbox/Models/cube.gltf");
    model->materials[0]->SetShader(AssetManager::Get().LoadAsset<Shader>("Engine/Shaders/Unlit.glsl"));
    model->materials[0]->SetTexture("mainTex", AssetManager::Get().LoadAsset<Texture2D>("Sandbox/Textures/Rock.jpg"));
    model->materials[0]->SetVector4("color", {1,1,1,1});

    for(int i = 0; i < 1000; i++){
        float posRange = 25;
        Transform t;

        float angle = 20.0f * i; 
        t.LocalPosition(Vector3(random(-posRange, posRange), random(0, posRange), random(-posRange, posRange)));
        t.LocalEulerAngles(Vector3(random(-180, 180), random(-180, 180), random(-180, 180)));
        transforms.push_back(t.GetLocalModelMatrix());
    }

    useInstancing = true;
}

void LoadModelSample::OnUpdate(float deltaTime){
    camMove.OnUpdate();
    //modelTransform.localEulerAngles(Vector3(0, Platform::GetTime() * 20, 0));
}   

void LoadModelSample::OnRender(float deltaTime){
    OD_PROFILE_SCOPE("LoadModel_2::OnRender");

    cam.SetPerspective(60, 0.1f, 1000.0f, Application::ScreenWidth(), Application::ScreenHeight());
    cam.view = math::inverse(camTransform.GetLocalModelMatrix());

    Graphics::Begin();
    Graphics::Clean(0.1f, 0.1f, 0.1f, 1);
    Graphics::SetCamera(cam);

    if(useInstancing){  
        //model->materials[0]->SetEnableInstancing(true);
        model->materials[0]->DisableKeyword("SKINNED");
        model->materials[0]->EnableKeyword("INSTANCING");
        Graphics::DrawMeshInstancing(*model->meshs[0], *model->materials[0], &transforms[0], transforms.size());
    } else {
        model->materials[0]->DisableKeyword("SKINNED");
        model->materials[0]->DisableKeyword("INSTANCING");
        for(auto i: transforms){
            Graphics::DrawMesh(*model->meshs[0], *model->materials[0], i);
        }
    }
    
    Graphics::End();
}

void LoadModelSample::OnGUI(){
    //static bool show;
    //ImGui::ShowDemoWindow(&show);

    ImGui::Begin("Load Model Test");

    ImGui::Checkbox("Use Instancing", &useInstancing);
    ImGui::Spacing();
    
    ImGui::Text("DrawCalls: %d", Graphics::GetStats().drawCalls);
    //ImGui::Text("Vertices: %dk", Graphics::GetVerticesCount() / 1000);
    //ImGui::Text("Tris: %dk", Graphics::GetTrisCount() / 1000);
    
    if(Graphics::GetStats().vertices >= 1000000){
        ImGui::Text("Vertices: %.1fM", Graphics::GetStats().vertices / 1000000.0f);
    } else if(Graphics::GetStats().vertices >= 1000){
        ImGui::Text("Vertices: %.1fk", Graphics::GetStats().vertices / 1000.0f);
    } else {
        ImGui::Text("Vertices: %d", Graphics::GetStats().vertices);
    }

    if(Graphics::GetStats().tris >= 1000000){
        ImGui::Text("Tris: %.1fM", Graphics::GetStats().tris / 1000000.0f);
    } else if(Graphics::GetStats().tris >= 1000){
        ImGui::Text("Tris: %.1fk", Graphics::GetStats().tris / 1000.0f);
    } else {
        ImGui::Text("Tris: %d", Graphics::GetStats().tris);
    }
    
    ImGui::End();
}

void LoadModelSample::OnResize(int width, int height){}
void LoadModelSample::OnExit(){}