#pragma once
#include <OD/Core/Module.h>
#include <OD/Base.h>
#include "Standard/UI/UI.h"

namespace OD{
    class Texture2D;
    class Material;
    class Font;
};

using namespace Standard;

struct UISample: public OD::Module {
    OD::Ref<OD::Texture2D> panelSprite;
    OD::Ref<OD::Font> font;
    OD::Ref<OD::Material> fontMat;

    UI::UIContext context;

    UISample(){ name = "UISample"; }
    void OnInit() override;
    void OnUpdate(float deltaTime) override;
    void OnRender(float deltaTime) override;
    void OnGUI() override;
    void OnResize(int width, int height) override;
    void OnExit() override;
};