#pragma once
#include <OD/OD.h>
#include "Ultis/CameraMovement.h"

using namespace OD;

struct LoadModelSample: OD::Module {
    Ref<Model> model;
    Ref<SubShader> shader;
    Transform camTransform;
    Camera cam;
    CameraMovement camMove;
    std::vector<Matrix4> transforms;

    bool useInstancing = true;

    void OnInit() override;
    void OnUpdate(float deltaTime) override; 
    void OnRender(float deltaTime) override;
    void OnGUI() override;
    void OnResize(int width, int height) override;
    void OnExit() override;
    
};