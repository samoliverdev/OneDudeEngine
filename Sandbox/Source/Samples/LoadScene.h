#pragma once
#include <OD/OD.h>

using namespace OD;

struct LoadSceneSample: public OD::Module {
    void OnInit() override;
    void OnUpdate(float deltaTime) override;
    void OnRender(float deltaTime) override;
    void OnGUI() override;
    void OnResize(int width, int height) override;
    void OnExit() override;
};