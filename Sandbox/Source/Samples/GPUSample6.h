#pragma once

#include <OD/Core/Module.h>

struct GPUSample6 final : public OD::Module {
    GPUSample6(){ name = "GPUSample6 - Framebuffer Blit"; }

    void OnInit() override;
    void OnUpdate(float deltaTime) override;
    void OnRender(float deltaTime) override;
    void OnExit() override;
    void OnGUI() override;
    void OnResize(int width, int height) override;
};
