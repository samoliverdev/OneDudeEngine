#pragma once

#include <OD/OD.h>
#include <OD/RenderPipeline/CommandBuffer.h>
#include <chrono>
#include <functional>
#include <atomic>
#include <algorithm>
#include <execution>

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

        clock_t start, end;
        start = clock();

        const int SIZE = 100000000;
        std::vector<int> arr;
        arr.resize(SIZE);

        #if 1 // This is Very more fast
        for(int i = 0; i < SIZE; i++){
            arr[i] += 20;
        }
        #else
        /*std::for_each(
            std::execution::par,
            arr.begin(),
            arr.end(),
            [&](auto& component_set){
                component_set += 20;
        });*/

        //std::atomic<int> counter(0);
        JobSystem::Dispatch(SIZE, SIZE/4, [&](JobDispatchArgs args){
            //LogInfo("JobIndex: %d, Group Index: %d", args.jobIndex, args.groupIndex);

            //unsigned int unique = counter.fetch_add(1, std::memory_order_relaxed); //This is more slow then use a loop without JobSystem
            //arr[counter] += 20;
            //counter.fetch_add(1); // Slow Too

            arr[args.jobIndex] += 20; //Without add is more fast then using atomic
        });
        JobSystem::Wait();
        #endif

        end = clock();

        double time_taken = double(end - start) / double(CLOCKS_PER_SEC);
        std::cout << "Time taken by program is : " << std::fixed << time_taken << std::setprecision(5);
        std::cout << " sec " << std::endl;

        std::vector<std::string> keywords{
            "Fade", "Skinned"
        };
        meshShader = Shader::CreateFromFile("res/Game/Shaders/test.glsl", keywords);
        fontShader = Shader::CreateFromFile("res/Engine/Shaders/Font.glsl");

        font = Font::CreateFromFile("res/Engine/Fonts/OpenSans/static/OpenSans_Condensed-Bold.ttf");

        LogInfo("-------------ShaderVariantCreateTest------------");
        std::vector<std::vector<std::string>> multCompile{
            std::vector<std::string>{ "Default", "Skinned", "Instancing" },
            std::vector<std::string>{ "Opaque", "Fade"},
            std::vector<std::string>{ "White", "Black"},
            std::vector<std::string>{ "Shadow_X1", "Shadow_X2", "Shadow_X3", "Shadow_X4"},
        };

        std::vector<std::string> combinations;
        Combine(multCompile, std::string(""), combinations);
        for(std::string s: combinations){
            LogInfo("%s", s.c_str());
        }
    }

    void Combine(std::vector<std::vector<std::string>> terms, std::string accum, std::vector<std::string>& combinations){
        bool last = (terms.size() == 1);
        int n = terms[0].size();
        for(int i = 0; i < n; i++){
            std::string item = accum + "_" + terms[0][i];
            if(last){
                combinations.push_back(item);
            } else{
                auto newTerms = terms;
                newTerms.erase(newTerms.begin());
                Combine(newTerms, item, combinations);
            }
        }
        /*for i in range(n):
            item = accum + '_' + terms[0][i]
            if last:
                combinations.append(item)
            else:
                combine(terms[1:], item)*/
    }

    void OnUpdate(float deltaTime) override {
        if(Input::IsKey(KeyCode::D)){ 
            LogInfo("Pressing key: D"); 
        }

        if(Input::IsKeyDown(KeyCode::R)){
            LogInfo("Reloading Shader");
            meshShader->Reload();
        }
    }   

    void OnRender(float deltaTime) override {
        Graphics::Begin();

        Graphics::Clean(0.1f, 0.1f, 0.1f, 1);
        Graphics::SetBlend(false);

        Camera cam = {Matrix4Identity, Matrix4Identity};
        Graphics::SetCamera(cam);

        //Renderer::SetRenderMode(Renderer::RenderMode::WIREFRAME);

        Graphics::SetDefaultShaderData(*meshShader, Matrix4Identity);
        Graphics::DrawMesh(mesh);

        /////////// Render Text ///////////

        Graphics::SetBlend(true);
        Graphics::SetBlendFunc(BlendMode::SRC_ALPHA, BlendMode::ONE_MINUS_SRC_ALPHA);
        
        cam = {Matrix4Identity, math::ortho(0.0f, (float)Application::ScreenWidth(), 0.0f, (float)Application::ScreenHeight(), -10.0f, 10.0f)};
        Graphics::SetCamera(cam);

        Graphics::DrawText(*font, *fontShader, "This is sample text", Vector3(25.0f, 25.0f, 0), 1.0f, Vector3(0.5f, 0.8f, 0.2f));
        Graphics::DrawText(*font, *fontShader, "(C) LearnOpenGL.com", Vector3(Application::ScreenWidth()-260, Application::ScreenHeight()-30, 0), 0.5f, Vector3(0.3, 0.7f, 0.9f));

        Graphics::End();
    }

    void OnGUI() override {
        static bool show;
        ImGui::ShowDemoWindow(&show);
    }

    void OnResize(int width, int height) override {}
    void OnExit() override {}
};