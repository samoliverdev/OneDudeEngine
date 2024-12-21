#pragma once
#include <OD/OD.h>
#include "Ultis/CameraMovement.h"

using namespace OD;

struct NavmeshSample: OD::Module {
    Entity camera;
    EntityId navmeshAgentEntity;
    EntityId targetPosEntity;

    void OnInit() override;
    void OnUpdate(float deltaTime) override;  
    void OnRender(float deltaTime) override;
    void OnGUI() override;
    void OnResize(int width, int height) override;
    void OnExit() override;
};