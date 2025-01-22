#include "UberShader.h"

void UberShaderSample::OnInit(){
    LogInfo("Game Init");

    modelTransform.LocalPosition(Vector3Zero);
    camTransform.LocalPosition(Vector3(0, 2, 10));
    camTransform.LocalEulerAngles(Vector3(-25, 0, 0));

    camMove.transform = &camTransform;

    model = AssetManager::Get().LoadAsset<Model>("Sandbox/Models/suzane.obj");
    uberShader = new MultiCompileShader("Sandbox/Shaders/UberShaderTest.glsl");
    uberShader->EnableKeyword("Fade");
    uberShader->EnableKeyword("Instancing");
}

void UberShaderSample::OnUpdate(float deltaTime){
    camMove.OnUpdate();
    //modelTransform.LocalEulerAngles(Vector3(0, Platform::GetTime() * 40, 0));
}   

void UberShaderSample::OnRender(float deltaTime){
    cam.SetPerspective(60, 0.1f, 1000.0f, Application::ScreenWidth(), Application::ScreenHeight());
    cam.view = math::inverse(camTransform.GetLocalModelMatrix());

    Graphics::Begin();
    Graphics::Clean(0.1f, 0.1f, 0.1f, 1);
    Graphics::SetCamera(cam);

    Matrix4 m1 = math::translate(Matrix4Identity, Vector3(2, 0, 0));
    Matrix4 m2 = math::translate(Matrix4Identity, Vector3(-2, 0, 0));

    uberShader->EnableKeyword("COLOR1");
    uberShader->SetCurrentShader();
    //Graphics::SetDefaultShaderData(*uberShader->GetCurrentShader(), m1);
    SubShader::Bind(*uberShader->GetCurrentShader());
    Graphics::SetProjectionViewMatrix(*uberShader->GetCurrentShader());
    Graphics::SetModelMatrix(*uberShader->GetCurrentShader(), m1);
    Graphics::DrawMeshRaw(*model->meshs[0]);

    uberShader->DisableKeyword("COLOR1");
    uberShader->SetCurrentShader();
    //Graphics::SetDefaultShaderData(*uberShader->GetCurrentShader(), m2);
    SubShader::Bind(*uberShader->GetCurrentShader());
    Graphics::SetProjectionViewMatrix(*uberShader->GetCurrentShader());
    Graphics::SetModelMatrix(*uberShader->GetCurrentShader(), m2);
    Graphics::DrawMeshRaw(*model->meshs[0]);

    Graphics::End();
}

void UberShaderSample::OnGUI(){}
void UberShaderSample::OnResize(int width, int height){}
void UberShaderSample::OnExit(){}