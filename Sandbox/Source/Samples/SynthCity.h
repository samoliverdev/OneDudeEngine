#pragma once
#include <OD/OD.h>

using namespace OD;

struct SynthCitySample: OD::Module {
    //CameraMovement camMove;
    Entity camera;

    void OnInit() override;
    void OnUpdate(float deltaTime) override;
    void OnRender(float deltaTime) override;
    void OnGUI() override;
    void OnResize(int width, int height) override;
    void OnExit() override;
};