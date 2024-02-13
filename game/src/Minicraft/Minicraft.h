#pragma once

#include <OD/OD.h>
#include "Ultis/CameraMovement.h"
#include "Ultis/Ultis.h"
#include "ChunkComponent.h"
#include "WorldManagerSystem.h"

using namespace OD;

struct Minicraft: public OD::Module {
    void OnInit() override;
    void OnUpdate(float deltaTime) override; 
    void OnRender(float deltaTime) override;
    void OnGUI() override;
    void OnResize(int width, int height) override;
    void OnExit() override;
};