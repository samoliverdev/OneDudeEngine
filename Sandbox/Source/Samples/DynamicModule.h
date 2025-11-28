#pragma once
#include <OD/Scene/Scene.h>
#include <OD/Core/Module.h>

using namespace OD;

struct PhysicsCubeS;

struct DynamicModuleSample: OD::Module {
    //CameraMovement camMove;
    Entity camera;
    Module* currentModule = nullptr;
    void* currentDll = nullptr;

    DynamicModuleSample(){ name = "DynamicModuleSample"; }
    void OnInit() override;
    void OnUpdate(float deltaTime) override;
    void OnRender(float deltaTime) override;
    void OnGUI() override;
    void OnResize(int width, int height) override;
    void OnExit() override;
};