#include "Light.h"
#include <OD/Graphics/Model.h>
#include <OD/Graphics/Graphics.h>
#include <OD/Core/Application.h>
#include <assert.h>

void LightSample::OnInit(){
    LogInfo("Game Init");

    /*lightModel = AssetManager::Get().LoadAsset<Model>("Sandbox/Models/sphere.obj");
    lightModel->materials[0]->SetShader(AssetManager::Get().LoadAsset<Shader>("Engine/Shaders/Unlit.glsl"));
    lightModel->materials[0]->SetVector4("color", Vector4(1, 1, 1, 1));
    lightTransform.LocalScale(Vector3(0.1f, 0.1f, 0.1f));
    lightTransform.LocalPosition(Vector3(-1, 2, 2));*/

    camMove.transform = &camTransform;

    modelTransform.LocalPosition(Vector3Zero);
    camTransform.LocalPosition(Vector3(0, 2, 4));
    camTransform.LocalEulerAngles(Vector3(-25, 0, 0));

    model = AssetManager::Get().LoadAsset<Model>("Sandbox/Models/suzane.obj");
    model->materials[0]->SetShader(AssetManager::Get().LoadAsset<Shader>("Sandbox/Shaders/light.glsl"));
    //model->materials[0]->SetTexture("texture1", AssetManager::Get().LoadAsset<Texture2D>("Sandbox/Textures/Rock.jpg"));
    model->materials[0]->SetVector3("color", Vector3(1.0f, 0.5f, 0.31f));
    model->materials[0]->SetVector3("lightColor", Vector3(1.0f, 1.0f, 1.0f));
    model->materials[0]->SetVector3("light_position", lightTransform.LocalPosition());
    model->materials[0]->SetVector3("viewPos", camTransform.LocalPosition());
    model->materials[0]->SetVector3("material_ambient", Vector3(0.3f, 0.3f, 0.31f));
    model->materials[0]->SetVector3("material_diffuse", Vector3(0.8f, 0.8f, 0.31f));
    model->materials[0]->SetVector3("material_specular", Vector3(0.5f, 0.5f, 0.5f));
    model->materials[0]->SetFloat("material_shininess", 32.0f);
    model->materials[0]->SetVector3("light_ambient",  Vector3(0.2f, 0.2f, 0.2f));
    model->materials[0]->SetVector3("light_diffuse",  Vector3(0.5f, 0.5f, 0.5f)); // darken diffuse light a bit
    model->materials[0]->SetVector3("light_specular", Vector3(1.0f, 1.0f, 1.0f)); 

    framebuffer = new Framebuffer(FramebufferType::Stand, Application::ScreenWidth(), Application::ScreenHeight());
    blitMat = CreateRef<Material>(Shader::CreateFromFile("Engine/Shaders/Blit.glsl"));
    fullScreenQuad = Mesh::FullScreenQuad();
}

void LightSample::OnUpdate(float deltaTime){
    camMove.OnUpdate();
    modelTransform.LocalEulerAngles(Vector3(0, Platform::GetTime() * 40, 0));
}   

void LightSample::OnRender(float deltaTime){
    cam.SetPerspective(60, 0.1f, 1000.0f, Application::ScreenWidth(), Application::ScreenHeight());
    cam.view = math::inverse(camTransform.GetLocalModelMatrix());

    model->materials[0]->SetVector3("viewPos", camTransform.LocalPosition());

    Graphics::Begin();
    //Graphics::Clean(0.1f, 0.1f, 0.1f, 1);
    Graphics::SetCamera(cam);

    Graphics::BeginFramebuffer(*framebuffer, Vector4(0.1f, 0.1f, 0.1f, 1));
    Graphics::Clean(0.1f, 0.1f, 0.1f, 1);
        Graphics::DrawModel(*model, modelTransform.GetLocalModelMatrix());
        for(unsigned int i = 0; i < 10; i++){
            modelTransform.LocalPosition(cubePositions[i]);
            float angle = 20.0f * i; 
            modelTransform.LocalEulerAngles(Vector3(angle*1, angle*0.3f, angle*0.5f));
            Graphics::DrawModel(*model, modelTransform.GetLocalModelMatrix());
        }
    Graphics::EndFramebuffer();

    Graphics::BeginRenderToScreen(Vector4(0.1f, 0.1f, 0.1f, 1));
        blitMat->SetTexture("mainTex", framebuffer, 0);
        Graphics::DrawMesh(*fullScreenQuad, *blitMat, Matrix4Identity);

        /*Graphics::DrawModel(*model, modelTransform.GetLocalModelMatrix());
        //Graphics::DrawModel(*lightModel, lightTransform.GetLocalModelMatrix());
        for(unsigned int i = 0; i < 10; i++){
            modelTransform.LocalPosition(cubePositions[i]);
            float angle = 20.0f * i; 
            modelTransform.LocalEulerAngles(Vector3(angle*1, angle*0.3f, angle*0.5f));
            Graphics::DrawModel(*model, modelTransform.GetLocalModelMatrix());
        }*/

        /*Platform::ImguiBegin();
        OnGUI();
        Platform::End();*/
    Graphics::EndRenderToScreen();
    
    Graphics::End();
}

void LightSample::OnGUI(){
    ImGui::Begin("Triangle Position/Color");

    static float translation[] = {0.0, 0.0, 0.0};
    ImGui::SliderFloat3("position", translation, -5, 5);
    lightTransform.LocalPosition(Vector3(translation[0], translation[1], translation[2]));

    static float color[4] = { 1.0f,1.0f,1.0f,1.0f };
    ImGui::ColorEdit3("color", color);
    
    model->materials[0]->SetVector3("light_diffuse",  Vector3(color[0], color[1], color[2]));
    model->materials[0]->SetVector3("light_position", lightTransform.LocalPosition());
    
    ImGui::End();
}

void LightSample::OnResize(int width, int height){

}

void LightSample::OnExit(){
    delete framebuffer;
}