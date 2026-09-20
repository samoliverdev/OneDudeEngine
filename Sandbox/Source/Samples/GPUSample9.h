#pragma once
#include <OD/Core/Module.h>

struct GPUSample9 final : public OD::Module {
    GPUSample9(){ name = "GPUSample9 - IBL Specular Prefilter and BRDF"; }
    void OnInit() override;
    void OnUpdate(float deltaTime) override;
    void OnRender(float deltaTime) override;
    void OnExit() override;
    void OnGUI() override;
    void OnResize(int width, int height) override;
};
