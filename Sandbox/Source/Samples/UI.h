#pragma once
#include <OD/Core/Module.h>
#include <OD/Base.h>

namespace OD{
    class Texture2D;
    class Material;
    class Font;
};

struct UISample: public OD::Module {
    OD::Ref<OD::Texture2D> panelSprite;
    OD::Ref<OD::Font> font;
    OD::Ref<OD::Material> fontMat;

    UISample(){ name = "UISample"; }
    void OnInit() override;
    void OnUpdate(float deltaTime) override;
    void OnRender(float deltaTime) override;
    void OnGUI() override;
    void OnResize(int width, int height) override;
    void OnExit() override;
};