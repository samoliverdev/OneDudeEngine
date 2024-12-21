#pragma once
#include <OD/OD.h>
#include <OD/Graphics/UniformBuffer.h>
#include "Ultis/CameraMovement.h"
#include "Ultis/Ultis.h"

using namespace OD;

struct UniformBufferSample: OD::Module {
    Ref<Model> model;
    Ref<Shader> shader;
    Ref<UniformBuffer> cBuffer;

    Transform camTransform;
    Camera cam;
    CameraMovement camMove;

    std::vector<Matrix4> transforms;

    void OnInit() override;
    void OnUpdate(float deltaTime) override;
    void OnRender(float deltaTime) override;
    void OnGUI() override;
    void OnResize(int width, int height) override;
    void OnExit() override;
};