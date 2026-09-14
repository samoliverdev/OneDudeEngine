#pragma once
#include <OD/Core/Module.h>

struct GPUSample3: public OD::Module {
    GPUSample3(){ name = "GPUSample3"; }
    void OnInit() override;
    void OnUpdate(float deltaTime) override;
    void OnRender(float deltaTime) override;
    void OnGUI() override;
    void OnResize(int width, int height) override;
    void OnExit() override;
};