#pragma once
#include <OD/Core/Module.h>
#include <OD/Graphics/Camera.h>
#include "Ultis/CameraMovement.h"

namespace OD{
    class Model;
    class SubShader;
}

using namespace OD;

struct LoadModelSample: OD::Module {
    Ref<Model> model;
    Ref<SubShader> shader;
    Transform camTransform;
    Camera cam;
    CameraMovement camMove;
    std::vector<Matrix4> transforms;

    bool useInstancing = false;

    void OnInit() override;
    void OnUpdate(float deltaTime) override; 
    void OnRender(float deltaTime) override;
    void OnGUI() override;
    void OnResize(int width, int height) override;
    void OnExit() override;
    
};