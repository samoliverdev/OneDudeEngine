#pragma once

#include <OD/OD.h>
#include <chrono>
#include <functional>

using namespace OD;

void Spin(float milliseconds){
    using namespace std;

	milliseconds /= 1000.0f;
	chrono::high_resolution_clock::time_point t1 = chrono::high_resolution_clock::now();
	double ms = 0;
	while (ms < milliseconds){
		chrono::high_resolution_clock::time_point t2 = chrono::high_resolution_clock::now();
		chrono::duration<double> time_span = chrono::duration_cast<chrono::duration<double>>(t2 - t1);
		ms = time_span.count();
	}
}

struct Data{
    enum class Type{Int, Float};

    Type type;
    const char* name;

    float* fValue;
    int* iValue;
};

struct Datas{
    void AddIntMember(int* value, const char* name){
        Data d;
        d.type = Data::Type::Int;
        d.iValue = value;
        d.name = name;
        datas.push_back(d);
    }

    void AddFloatMember(float* value, const char* name){
        Data d;
        d.type = Data::Type::Float;
        d.fValue = value;
        d.name = name;
        datas.push_back(d);
    }

    void Add(){
        for(auto i: datas){
            if(i.type == Data::Type::Float) *i.fValue += 5;
            if(i.type == Data::Type::Int) *i.iValue += 5;
        }
    }

private:
    std::vector<Data> datas;
};

struct CompTest1{
    int a;
    float b;

    void Serialize(Datas& desc){
        desc.AddIntMember(&a, "a");
        desc.AddFloatMember(&b, "b");
    }
};

struct BaseMesh_1: OD::Module {
    Mesh mesh;
    
    Ref<Shader> meshShader;
    Ref<Shader> fontShader;

    Ref<Font> font;

    void OnInit() override {
        LogInfo("Game Init");

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
        mesh.UpdateMesh();

        /*
        mesh.indices.clear();
        mesh.indices.push_back(0);
        mesh.indices.push_back(1);
        mesh.indices.push_back(3);
        mesh.UpdateMesh();
        */

        /*JobSystem::Initialize();
        JobSystem::Execute([] { Spin(100); });
		JobSystem::Execute([] { Spin(100); });
		JobSystem::Execute([] { Spin(100); });
		JobSystem::Execute([] { Spin(100); });
		JobSystem::Execute([] { Spin(100); });
		JobSystem::Execute([] { Spin(100); });
		JobSystem::Wait();

        int arr[10000];
        JobSystem::Dispatch(10000, 10000/4, [&arr](JobDispatchArgs args){
            LogInfo("JobIndex: %d, Group Index: %d", args.jobIndex, args.groupIndex);
            arr[args.jobIndex] += 20;
        });
        JobSystem::Wait();*/

        meshShader = Shader::CreateFromFile("res/shaders/test.glsl");
        fontShader = Shader::CreateFromFile("res/Builtins/Shaders/Font.glsl");

        font = Font::CreateFromFile("res/Builtins/Fonts/OpenSans/static/OpenSans_Condensed-Bold.ttf");
    }

    void OnUpdate(float deltaTime) override {
        if(Input::IsKey(KeyCode::D)){ LogInfo("Pressing key: D"); }
    }   

    void OnRender(float deltaTime) override {
        Renderer::Begin();

        Renderer::Clean(0.1f, 0.1f, 0.1f, 1);
        Renderer::SetBlend(false);

        Camera cam = {Matrix4::identity, Matrix4::identity};
        Renderer::SetCamera(cam);

        //Renderer::SetRenderMode(Renderer::RenderMode::WIREFRAME);
        Renderer::DrawMesh(mesh, Matrix4::identity, *meshShader);

        /////////// Render Text ///////////

        Renderer::SetBlend(true);
        Renderer::SetBlendFunc(BlendMode::SRC_ALPHA, BlendMode::ONE_MINUS_SRC_ALPHA);
        
        cam = {Matrix4::identity, Matrix4::Ortho(0.0f, Application::screenWidth(), 0.0f, Application::screenHeight(), -10, 10)};
        Renderer::SetCamera(cam);

        Renderer::DrawText(*font, *fontShader, "This is sample text", Vector3(25.0f, 25.0f, 0), 1.0f, Vector3(0.5f, 0.8f, 0.2f));
        Renderer::DrawText(*font, *fontShader, "(C) LearnOpenGL.com", Vector3(Application::screenWidth()-260, Application::screenHeight()-30, 0), 0.5f, Vector3(0.3, 0.7f, 0.9f));

        Renderer::End();
    }

    void OnGUI() override {
        //static bool show;
        //ImGui::ShowDemoWindow(&show);
    }

    void OnResize(int width, int height) override {}
};