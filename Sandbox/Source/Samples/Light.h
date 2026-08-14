#pragma once
#include <OD/Core/Module.h>
#include <OD/Core/Transform.h>
#include <OD/Graphics/Camera.h>
#include "Ultis/CameraMovement.h"

namespace OD{
    class Model;
    class SubShader;
    class Mesh;
    class Framebuffer;
    class Material;
}

using namespace OD;

struct LightSample: OD::Module {
    Ref<Model> model;
    Ref<SubShader> shader;
    Transform modelTransform;
    Transform camTransform;
    Camera cam;
    CameraMovement camMove;

    Ref<Model> lightModel;
    Transform lightTransform;

    Ref<Framebuffer> framebuffer;
    Ref<Material> blitMat;
    Ref<Mesh> fullScreenQuad;

    Vector3 cubePositions[10] = {
        //Vector3( 0.0f,  0.0f,  0.0f), 
        Vector3( 2.0f,  5.0f, -15.0f), 
        Vector3(-1.5f, -2.2f, -2.5f),  
        Vector3(-3.8f, -2.0f, -12.3f),  
        Vector3( 2.4f, -0.4f, -3.5f),  
        Vector3(-1.7f,  3.0f, -7.5f),  
        Vector3( 1.3f, -2.0f, -2.5f),  
        Vector3( 1.5f,  2.0f, -2.5f), 
        Vector3( 1.5f,  0.2f, -1.5f), 
        Vector3(-1.3f,  1.0f, -1.5f)  
    };

    LightSample(){ name = "LightSample"; }
    void OnInit() override;
    void OnUpdate(float deltaTime) override;
    void OnRender(float deltaTime) override;
    void OnGUI() override;
    void OnResize(int width, int height) override;
    void OnExit() override;
};