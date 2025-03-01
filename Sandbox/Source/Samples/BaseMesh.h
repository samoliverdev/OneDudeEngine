#pragma once
#include <OD/OD.h>
//#include <taskflow/taskflow.hpp> 

struct BaseMeshSample: OD::Module {
    OD::Mesh mesh;
    //Ref<SubShader> meshShader;
    //Ref<SubShader> fontShader;
    //Ref<Font> font;

    OD::Ref<OD::Material> meshMat;

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