#pragma once

#include <OD/Core/Module.h>

struct GPUSample10 : public OD::Module {
    GPUSample10() { name = "GPUSample10"; }

    void OnInit() override;
    void OnUpdate(float deltaTime) override;
    void OnRender(float deltaTime) override;
    void OnGUI() override;
    void OnResize(int width, int height) override;
    void OnExit() override;
};
