#include "BaseMesh.h"

void BaseMeshSample::OnInit(){
    LogInfo("Game Init");
    OD::Application::Vsync(false);

    mesh.vertices.push_back(Vector3(0.5f, 0.5f, 0));
    mesh.vertices.push_back(Vector3(0.5f, -0.5f, 0));
    mesh.vertices.push_back(Vector3(-0.5f, -0.5f, 0));
    mesh.vertices.push_back(Vector3(-0.5f, 0.5f, 0));
    mesh.indices.push_back(0);
    mesh.indices.push_back(1);
    mesh.indices.push_back(3);
    mesh.indices.push_back(1);
    mesh.indices.push_back(2);
    mesh.indices.push_back(3);
    mesh.Submit();

    meshMat = CreateRef<Material>(Shader::CreateFromFile("Sandbox/Shaders/test.glsl"));
}

void BaseMeshSample::OnUpdate(float deltaTime){
    if(Input::IsKey(KeyCode::D)){ 
        LogInfo("Pressing key: D"); 
    }

    if(Input::IsKeyDown(KeyCode::R)){
        LogInfo("Reloading Shader");
        //meshShader->Reload();
    }
}   

void BaseMeshSample::OnRender(float deltaTime){
    Graphics::Begin();

    Camera cam = {Matrix4Identity, Matrix4Identity};
    Graphics::SetCamera(cam);
    Graphics::Clean(0.1f, 0.1f, 0.1f, 1);
    Graphics::DrawMesh(mesh, *meshMat, Matrix4Identity);
    
    Graphics::End();
}

void BaseMeshSample::OnGUI(){
    static bool show;
    ImGui::ShowDemoWindow(&show);
}

void BaseMeshSample::OnResize(int width, int height){}
void BaseMeshSample::OnExit(){}