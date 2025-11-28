#pragma once
#include <OD/Core/Module.h>

using namespace OD;

struct SponzaSample: public OD::Module {
    SponzaSample(){ name = "SponzaSample"; }
    void OnInit() override;
    void OnUpdate(float deltaTime) override;
    void OnRender(float deltaTime) override;
    void OnGUI() override;
    void OnResize(int width, int height) override;
    void OnExit() override;
};