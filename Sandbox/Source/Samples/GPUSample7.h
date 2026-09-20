#pragma once
#include <OD/Core/Module.h>

struct GPUSample7: public OD::Module {
    GPUSample7(){ name = "GPUSample7 - CopyTexture"; }
    void OnInit() override;
    void OnUpdate(float) override {}
    void OnRender(float) override;
    void OnGUI() override {}
    void OnResize(int, int) override {}
    void OnExit() override {}
};
