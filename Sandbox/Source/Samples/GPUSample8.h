#pragma once
#include <OD/Core/Module.h>

struct GPUSample8 final : public OD::Module {
    GPUSample8(){ name = "GPUSample8 - IBL Specular Prefilter"; }
    void OnInit() override;
    void OnUpdate(float deltaTime) override;
    void OnRender(float deltaTime) override;
    void OnExit() override;
    void OnGUI() override;
    void OnResize(int width, int height) override;
};
