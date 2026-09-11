#pragma once
#include <OD/Core/Module.h>
#include <OD/Graphics/Camera.h>
#include "Ultis/CameraMovement.h"

namespace OD{
    class Model;
    class SubShader;
    class InstancingBuffer;
}

using namespace OD;

struct LoadModelSample: OD::Module {
    Ref<Model> model;
    Ref<SubShader> shader;
    Ref<InstancingBuffer> buffer;
    Ref<InstancingBuffer> buffer2;
    Transform camTransform;
    Camera cam;
    CameraMovement camMove;
    std::vector<Matrix4> transforms;
    std::vector<Matrix4x3> transforms2;

    bool useInstancing = false; //false;
    bool useInstancingBuffer = true;
    bool useMatrix4x3 = false;

    LoadModelSample(){ name = "LoadModelSample"; }
    void OnInit() override;
    void OnUpdate(float deltaTime) override; 
    void OnRender(float deltaTime) override;
    void OnGUI() override;
    void OnResize(int width, int height) override;
    void OnExit() override;
    
};