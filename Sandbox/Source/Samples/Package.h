#pragma once
#include <OD/Core/Module.h>

using namespace OD;

struct PackageSample: public OD::Module {
    PackageSample(){ name = "PackgeSample"; }
    void OnInit() override;
    void OnUpdate(float deltaTime) override;
    void OnRender(float deltaTime) override;
    void OnGUI() override;
    void OnResize(int width, int height) override;
    void OnExit() override;
};