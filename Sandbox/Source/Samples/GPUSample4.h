#pragma once
#include "OD/Core/Module.h"

struct GPUSample4 final : public OD::Module {
    GPUSample4(){ name = "GPUSample4"; }
    void OnInit() override;
    void OnUpdate(float deltaTime) override;
    void OnRender(float deltaTime) override;
    void OnExit() override;
    void OnGUI() override;
    void OnResize(int width, int height) override;
};
