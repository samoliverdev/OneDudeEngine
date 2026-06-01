#pragma once
#include <OD/Core/Module.h>
#include <OD/Base.h>

namespace OD{
    class Texture2D;
    class Material;
};

struct UISample: public OD::Module {
    OD::Ref<OD::Texture2D> panelSprite;

    UISample(){ name = "UISample"; }
    void OnInit() override;
    void OnUpdate(float deltaTime) override;
    void OnRender(float deltaTime) override;
    void OnGUI() override;
    void OnResize(int width, int height) override;
    void OnExit() override;
};