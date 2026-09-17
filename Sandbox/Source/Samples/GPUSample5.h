#pragma once
#include "OD/Core/Module.h"

struct GPUSample5 final : public OD::Module {
    GPUSample5(){ name = "GPUSample5"; }
    void OnInit() override;
    void OnUpdate(float deltaTime) override;
    void OnRender(float deltaTime) override;
    void OnExit() override;
    void OnGUI() override;
    void OnResize(int width, int height) override;
};
