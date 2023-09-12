#pragma once

#include <OD/OD.h>
#include <chrono>

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
    char* name;

    float* fValue;
    int* iValue;
};

struct Datas{
    void AddIntMember(int* value, char* name){
        Data d;
        d.type = Data::Type::Int;
        d.iValue = value;
        d.name = name;
        datas.push_back(d);
    }

    void AddFloatMember(float* value, char* name){
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
    Ref<Shader> shader;

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

        JobSystem::Initialize();
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
        JobSystem::Wait();

        shader = Shader::CreateFromFile("res/shaders/test.glsl");
    }

    void OnUpdate(float deltaTime) override {
        if(Input::IsKey(KeyCode::D)){ LogInfo("Pressing key: D"); }
    }   

    void OnRender(float deltaTime) override {
        Renderer::Begin();
        Renderer::Clean(0.1f, 0.1f, 0.1f, 1);

        //Renderer::SetRenderMode(Renderer::RenderMode::WIREFRAME);
        Renderer::DrawMesh(mesh, Matrix4::identity, *shader);

        Renderer::End();
    }

    void OnGUI() override {
        //static bool show;
        //ImGui::ShowDemoWindow(&show);
    }

    void OnResize(int width, int height) override {;
    }
};