#pragma once
#include <OD/Core/Module.h>
#include <OD/Scene/Scene.h>
#include "Ultis/CameraMovement.h"

using namespace OD;

struct NavmeshSample: OD::Module {
    Entity camera;
    Entity navmeshAgentEntity;
    Entity targetPosEntity;

    void OnInit() override;
    void OnUpdate(float deltaTime) override;  
    void OnRender(float deltaTime) override;
    void OnGUI() override;
    void OnResize(int width, int height) override;
    void OnExit() override;
};