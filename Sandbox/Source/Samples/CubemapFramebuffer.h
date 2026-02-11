#pragma once
#include <OD/Core/Module.h>
#include <OD/Graphics/Camera.h>
#include "Ultis/CameraMovement.h"

namespace OD{
    class Mesh;
    class Model;
    class SubShader;
    class InstancingBuffer;
    class Framebuffer;
}

using namespace OD;

struct CubemapFramebufferSample: OD::Module {
    Ref<Mesh> fullscreenQuadMesh;
    Ref<Mesh> skyMesh;
    Ref<Model> model;
    Ref<Model> model2;
    Ref<Framebuffer> tempFB;
    Ref<Framebuffer> cubeFB;
    Ref<Material> mat1;
    Ref<Material> mat2;
    Ref<Material> skyMat;
    Ref<Material> screenPassMat;

    Transform camTransform;
    Camera cam;
    CameraMovement camMove;

    float roughness = 0;
    float metalness = 1;

    CubemapFramebufferSample(){ name = "CubemapFramebufferSample"; }
    void OnInit() override;
    void OnUpdate(float deltaTime) override; 
    void OnRender(float deltaTime) override;
    void OnGUI() override;
    void OnResize(int width, int height) override;
    void OnExit() override;
};