#pragma once
#include <OD/Core/Module.h>
#include <OD/Graphics/Mesh.h>
//#include <taskflow/taskflow.hpp> 

namespace OD{
    class Mesh;
    class Font;
    class Material;
}

struct BaseMeshSample: OD::Module {
    OD::Mesh mesh;
    //Ref<SubShader> meshShader;
    //Ref<SubShader> fontShader;
    
    OD::Ref<OD::Font> font;
    OD::Ref<OD::Material> fontMat;

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