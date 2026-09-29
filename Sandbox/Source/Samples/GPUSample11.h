#pragma once
#include <OD/Core/Module.h>

struct GPUSample11 final : public OD::Module {
    GPUSample11(){ name = "GPUSample11 - Texture2DArray"; }
    void OnInit() override;
    void OnUpdate(float) override {}
    void OnRender(float) override;
    void OnGUI() override {}
    void OnResize(int, int) override {}
    void OnExit() override;
};
