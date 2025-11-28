#pragma once
#include <OD/Core/Module.h>

struct JointSample: public OD::Module {
    JointSample(){ name = "JointSample"; }
    void OnInit() override;
    void OnUpdate(float deltaTime) override;
    void OnRender(float deltaTime) override;
    void OnGUI() override;
    void OnResize(int width, int height) override;
    void OnExit() override;
};