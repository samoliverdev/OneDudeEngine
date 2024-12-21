#pragma once
#include <OD/OD.h>
//#include <taskflow/taskflow.hpp> 

using namespace OD;

struct BaseMeshSample: OD::Module {
    Mesh mesh;
    Ref<Shader> meshShader;
    Ref<Shader> fontShader;
    Ref<Font> font;

    /*tf::Executor executor;
    tf::Taskflow taskflow;
    bool executorEnd = false;*/

    void OnInit() override;
    void OnUpdate(float deltaTime) override;
    void OnRender(float deltaTime) override;
    void OnGUI() override;
    void OnResize(int width, int height) override;
    void OnExit() override;
};