#pragma once
#include <OD/OD.h>
#include <OD/Graphics/MultiCompileShader.h>
#include "Ultis/CameraMovement.h"
#include <assert.h>

using namespace OD;

struct UberShaderSample: OD::Module {
    Ref<Model> model;
    //Ref<Shader> shader;
    MultiCompileShader* uberShader;
    Transform modelTransform;
    Transform camTransform;
    Camera cam;
    CameraMovement camMove;

    void OnInit() override;
    void OnUpdate(float deltaTime) override;
    void OnRender(float deltaTime) override;
    void OnGUI() override;
    void OnResize(int width, int height) override;
    void OnExit() override;
};