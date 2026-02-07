#include "OD/pch.h"
#include "BaseMesh.h"
#include "ECSTest.h"
#include <OD/Graphics/Font.h>
#include <OD/Graphics/Material.h>
#include <OD/Graphics/Graphics.h>
#include <OD/Scene/Scene.h>
#include <OD/Scene/GroupOfComps.h>
#include <OD/Core/Input.h>
#include <OD/Core/Application.h>
#include <OD/Core/ImGui.h>
#include <OD/RenderPipeline/MeshRendererComponent.h>

ImFont* customFont = nullptr;

void BaseMeshSample::OnInit(){
    using CharacterType = OD::GroupOfComps<OD::TransformComponent, OD::MeshRendererComponent>;

    OD::Registry gg;
    auto character1 = CharacterType::Create(
        gg,
        OD::TransformComponent{},
        OD::MeshRendererComponent{}
    );
    auto character2 = CharacterType::Create(
        gg,
        [](auto& trans, auto& renderer){

        }
    );


    struct Position { float x, y; };
    struct Velocity { float dx, dy; };
    struct Name {std::string name; };

    registry reg;
    
    entity e1 = reg.create();
    reg.add<Position>(e1, 1.0f, 2.0f);
    reg.add<Velocity>(e1, 0.5f, 1.5f);
    //reg.add<Name>(e1, "e1");
    
    entity e2 = reg.create();
    reg.add<Position>(e2, 3.0f, 4.0f);
    reg.add<Velocity>(e2, 0.5f, 1.5f);
    reg.add<Name>(e2, "e2");
    
    // Iterate through all entities with both Position and Velocity
    auto moving = reg.View<Position, Velocity, Name>();
    for (entity e : moving) {
        auto& pos = reg.get<Position>(e);
        auto& vel = reg.get<Velocity>(e);
        auto& name = reg.get<Name>(e);
        pos.x += vel.dx;
        pos.y += vel.dy;
        LogInfo("------Name: {}", name.name.c_str());
    }
    
    LogInfo("Game Init");
    //Assert(false);
    //OD::Application::Vsync(false);

    mesh.vertices.push_back(OD::Vector3(0.5f, 0.5f, 0));
    mesh.vertices.push_back(OD::Vector3(0.5f, -0.5f, 0));
    mesh.vertices.push_back(OD::Vector3(-0.5f, -0.5f, 0));
    mesh.vertices.push_back(OD::Vector3(-0.5f, 0.5f, 0));
    mesh.uv.push_back(OD::Vector3(1, 1, 0));
    mesh.uv.push_back(OD::Vector3(1, 0, 0));
    mesh.uv.push_back(OD::Vector3(0, 0, 0));
    mesh.uv.push_back(OD::Vector3(0, 1, 0));
    mesh.indices.reserve(10);
    mesh.indices.push_back(0);
    mesh.indices.push_back(1);
    mesh.indices.push_back(3);
    mesh.indices.push_back(1);
    mesh.indices.push_back(2);
    mesh.indices.push_back(3);
    mesh.Submit();

    meshMat = OD::CreateRef<OD::Material>(OD::Shader::CreateFromFile("Sandbox/Shaders/test.glsl"));

    auto lit = OD::Shader::CreateFromFile("Engine/Shaders/Lit.glsl");

    font = OD::Font::CreateFromFile("Engine/Fonts/OpenSans/static/OpenSans_Condensed-MediumItalic.ttf");
    Assert(font != nullptr);
    fontMat = OD::CreateRef<OD::Material>(OD::Shader::CreateFromFile("Engine/Shaders/Font.glsl"));

    //ImGui::GetIO().Fonts->TexDesiredWidth = 2048*2;

    /*ImFontConfig cfg;
    cfg.OversampleH = 8;
    cfg.OversampleV = 8;
    cfg.PixelSnapH  = false;*/
    customFont = ImGui::GetIO().Fonts->AddFontFromFileTTF("Engine/Fonts/OpenSans/static/OpenSans-Regular.ttf", 28.0f/2);//, &cfg);

    //ImGui::GetIO().Fonts->Build();
}

void BaseMeshSample::OnUpdate(float deltaTime){
    if(OD::Input::IsKey(OD::KeyCode::D)){ 
        LogInfo("Pressing key: D"); 
    }

    if(OD::Input::IsKeyDown(OD::KeyCode::R)){
        LogInfo("Reloading Shader");
        //meshShader->Reload();
    }
}   

#undef DrawText

void BaseMeshSample::OnRender(float deltaTime){
    OD::Graphics::Begin();

    OD::Graphics::SetViewport(0, 0, OD::Application::ScreenWidth(), OD::Application::ScreenHeight());
    OD::Graphics::BeginRenderToScreen({0.5f, 0.1f, 0.1f, 1.0f});

    OD::Camera cam = {OD::Matrix4Identity, OD::Matrix4Identity};
    OD::Graphics::SetCamera(cam);

    OD::Graphics::DrawMesh(mesh, *meshMat, OD::math::translate(OD::Vector3(0.5f, 0, 0)));
    OD::Graphics::DrawMesh(mesh, *meshMat, OD::math::translate(OD::Vector3(-0.5f, 0, 0)));
    
    cam = {OD::Matrix4Identity, OD::math::ortho(0.0f, (float)OD::Application::ScreenWidth(), 0.0f, (float)OD::Application::ScreenHeight(), -10.0f, 10.0f)};
    OD::Graphics::SetCamera(cam);
    OD::Transform tt;
    tt.Position(OD::Vector3(25*2, 25*2, 0));
    tt.Scale(OD::Vector3(25*2));
    OD::Graphics::DrawText(*font, *fontMat, "(C) LearnOpenGL.com", tt.GetModelMatrix(), false, {});
    
    OD::Graphics::EndRenderToScreen();

    OD::Graphics::End();
}

void DrawMainMenu(bool& showSettings, bool& startGame, bool& quitGame){
    ImGuiIO& io = ImGui::GetIO();
    ImVec2 screenCenter = ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f); //io.DisplaySize;

    // Center the window
    ImVec2 windowSize(400, 300);
    ImVec2 windowPos =  ImVec2(screenCenter.x - windowSize.x * 0.5f, screenCenter.y - windowSize.y * 0.5f); //screenCenter - windowSize;
    ImGui::SetNextWindowPos(windowPos, ImGuiCond_Always);
    ImGui::SetNextWindowSize(windowSize, ImGuiCond_Always);

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 12.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.05f, 0.05f, 0.05f, 0.85f));

    ImGui::Begin("MainMenu", nullptr,
        ImGuiWindowFlags_NoDecoration |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoSavedSettings
    );

    // --- Title ---
    ImGui::SetCursorPosY(30);
    ImGui::PushFont(io.Fonts->Fonts[0]); // Use your large font here if you have one
    ImGui::SetCursorPosX((windowSize.x - ImGui::CalcTextSize("My Awesome Game").x) * 0.5f);
    ImGui::Text("My Awesome Game");
    ImGui::PopFont();

    ImGui::Dummy(ImVec2(0, 40)); // Spacer

    // --- Buttons ---
    float buttonWidth = 200;
    float buttonHeight = 40;
    float buttonX = (windowSize.x - buttonWidth) * 0.5f;

    ImGui::SetCursorPosX(buttonX);
    if (ImGui::Button("Play", ImVec2(buttonWidth, buttonHeight))) {
        startGame = true;
    }

    ImGui::SetCursorPosX(buttonX);
    if (ImGui::Button("Settings", ImVec2(buttonWidth, buttonHeight))) {
        showSettings = true;
    }

    ImGui::SetCursorPosX(buttonX);
    if (ImGui::Button("Exit", ImVec2(buttonWidth, buttonHeight))) {
        quitGame = true;
    }

    ImGui::End();
    ImGui::PopStyleColor();
    ImGui::PopStyleVar(2);

    // --- Settings Popup ---
    if (showSettings)
    {
        ImGui::OpenPopup("Settings");
        showSettings = false;
    }

    if (ImGui::BeginPopupModal("Settings", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        static float masterVolume = 1.0f;
        static float brightness = 1.0f;
        ImGui::Text("Audio");
        ImGui::SliderFloat("Master Volume", &masterVolume, 0.0f, 1.0f);
        ImGui::Separator();
        ImGui::Text("Video");
        ImGui::SliderFloat("Brightness", &brightness, 0.5f, 1.5f);

        ImGui::Dummy(ImVec2(0, 10));
        if (ImGui::Button("Close", ImVec2(120, 0))) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}

struct PlayerStatus{
    int health = 87;
    int armor  = 56;
    int ammo   = 30;
    int ammoReserve = 120;
};

void DrawHalfLife2HUD(const PlayerStatus& status){
    ImGuiIO& io = ImGui::GetIO();
    ImDrawList* drawList = ImGui::GetForegroundDrawList();

    ImVec2 screenSize = io.DisplaySize;
    const float padding = 20.0f;

    // HL2 orange color
    ImU32 hl2Orange = IM_COL32(255, 153, 0, 255);
    ImU32 bgColor   = IM_COL32(20, 20, 20, 120);

    // Font scaling
    float fontSize = 24.0f;
    ImGui::PushFont(io.Fonts->Fonts[0]);

    //----------------------------------------
    // Health and Armor Bars (bottom left)
    //----------------------------------------
    ImVec2 basePos(padding, screenSize.y - 100.0f);

    // Health background
    ImVec2 healthBgSize(200, 20);
    drawList->AddRectFilled(basePos, 
        ImVec2(basePos.x + healthBgSize.x, basePos.y + healthBgSize.y),
        bgColor, 4.0f);

    // Health bar
    float healthFrac = (float)status.health / 100.0f;
    drawList->AddRectFilled(basePos,
        ImVec2(basePos.x + healthBgSize.x * healthFrac, basePos.y + healthBgSize.y),
        hl2Orange, 4.0f);

    drawList->AddText(ImVec2(basePos.x, basePos.y - fontSize - 2), hl2Orange, "HEALTH");

    // Armor
    ImVec2 armorPos(basePos.x, basePos.y + 30);
    ImVec2 armorBgSize(200, 20);
    float armorFrac = (float)status.armor / 100.0f;

    drawList->AddRectFilled(armorPos,
        ImVec2(armorPos.x + armorBgSize.x, armorPos.y + armorBgSize.y),
        bgColor, 4.0f);
    drawList->AddRectFilled(armorPos,
        ImVec2(armorPos.x + armorBgSize.x * armorFrac, armorPos.y + armorBgSize.y),
        IM_COL32(0, 170, 255, 255), 4.0f);
    drawList->AddText(ImVec2(armorPos.x, armorPos.y - fontSize - 2), IM_COL32(0, 170, 255, 255), "SUIT");

    //----------------------------------------
    // Ammo (bottom right)
    //----------------------------------------
    ImVec2 ammoPos(screenSize.x - 250.0f, screenSize.y - 100.0f);
    char ammoText[64];
    sprintf(ammoText, "%d / %d", status.ammo, status.ammoReserve);
    drawList->AddText(ammoPos, hl2Orange, ammoText);

    //----------------------------------------
    // Crosshair (center)
    //----------------------------------------
    ImVec2 center(screenSize.x * 0.5f, screenSize.y * 0.5f);
    float crossSize = 6.0f;
    float thickness = 2.0f;
    drawList->AddLine(ImVec2(center.x - crossSize, center.y), ImVec2(center.x + crossSize, center.y), hl2Orange, thickness);
    drawList->AddLine(ImVec2(center.x, center.y - crossSize), ImVec2(center.x, center.y + crossSize), hl2Orange, thickness);

    //----------------------------------------
    // Optional subtle top label
    //----------------------------------------
    drawList->AddText(ImVec2(padding, 20), IM_COL32(255, 255, 255, 150), "City 17");

    ImGui::PopFont();
}

void DrawHalfLife2HUD2(const PlayerStatus& status)
{
    ImGuiIO& io = ImGui::GetIO();
    ImDrawList* drawList = ImGui::GetForegroundDrawList();

    ImVec2 screenSize = io.DisplaySize;
    const float padding = 20.0f;
    const float innerPadding = 10.0f;

    // Colors
    ImU32 hl2Orange = IM_COL32(255, 160, 0, 255);
    ImU32 hl2Blue   = IM_COL32(0, 170, 255, 255);
    ImU32 panelBg   = IM_COL32(0, 0, 0, 160);
    ImU32 barBg     = IM_COL32(0, 0, 0, 100);

    //----------------------------------------
    // Bottom-left panel (Health + Suit)
    //----------------------------------------
    ImVec2 panelSize(240, 80);
    ImVec2 panelPos(padding, screenSize.y - panelSize.y - padding);

    drawList->AddRectFilled(
        panelPos,
        ImVec2(panelPos.x + panelSize.x, panelPos.y + panelSize.y),
        panelBg, 6.0f
    );

    // HEALTH
    float healthFrac = (float)status.health / 100.0f;
    ImVec2 healthPos(panelPos.x + innerPadding, panelPos.y + innerPadding);
    ImVec2 healthBarSize(200, 16);

    // Bar background
    drawList->AddRectFilled(
        healthPos,
        ImVec2(healthPos.x + healthBarSize.x, healthPos.y + healthBarSize.y),
        barBg, 3.0f
    );
    // Filled bar
    drawList->AddRectFilled(
        healthPos,
        ImVec2(healthPos.x + healthBarSize.x * healthFrac, healthPos.y + healthBarSize.y),
        hl2Orange, 3.0f
    );

    // Label + number
    drawList->AddText(ImVec2(healthPos.x, healthPos.y - 20), hl2Orange, "HEALTH");
    char healthText[16];
    sprintf(healthText, "%d", status.health);
    drawList->AddText(ImVec2(healthPos.x + 210, healthPos.y - 2), hl2Orange, healthText);

    // SUIT
    float armorFrac = (float)status.armor / 100.0f;
    ImVec2 armorPos(panelPos.x + innerPadding, panelPos.y + 40);
    ImVec2 armorBarSize(200, 16);

    drawList->AddRectFilled(
        armorPos,
        ImVec2(armorPos.x + armorBarSize.x, armorPos.y + armorBarSize.y),
        barBg, 3.0f
    );
    drawList->AddRectFilled(
        armorPos,
        ImVec2(armorPos.x + armorBarSize.x * armorFrac, armorPos.y + armorBarSize.y),
        hl2Blue, 3.0f
    );

    drawList->AddText(ImVec2(armorPos.x, armorPos.y - 20), hl2Blue, "SUIT");
    char armorText[16];
    sprintf(armorText, "%d", status.armor);
    drawList->AddText(ImVec2(armorPos.x + 210, armorPos.y - 2), hl2Blue, armorText);

    //----------------------------------------
    // Bottom-right panel (Ammo)
    //----------------------------------------
    ImVec2 ammoPanelSize(180, 60);
    ImVec2 ammoPanelPos(screenSize.x - ammoPanelSize.x - padding, screenSize.y - ammoPanelSize.y - padding);

    drawList->AddRectFilled(
        ammoPanelPos,
        ImVec2(ammoPanelPos.x + ammoPanelSize.x, ammoPanelPos.y + ammoPanelSize.y),
        panelBg, 6.0f
    );

    ImVec2 ammoInner(ammoPanelPos.x + innerPadding, ammoPanelPos.y + innerPadding);
    drawList->AddText(ammoInner, hl2Orange, "AMMO");
    char ammoText[32];
    sprintf(ammoText, "%d / %d", status.ammo, status.ammoReserve);
    drawList->AddText(ImVec2(ammoInner.x, ammoInner.y + 25), hl2Orange, ammoText);

    //----------------------------------------
    // Center crosshair
    //----------------------------------------
    ImVec2 center(screenSize.x * 0.5f, screenSize.y * 0.5f);
    float crossSize = 6.0f;
    float thickness = 2.0f;
    drawList->AddLine(ImVec2(center.x - crossSize, center.y), ImVec2(center.x + crossSize, center.y), hl2Orange, thickness);
    drawList->AddLine(ImVec2(center.x, center.y - crossSize), ImVec2(center.x, center.y + crossSize), hl2Orange, thickness);
}

////////////////////////
/*static const float BASE_WIDTH  = 1920.0f;
static const float BASE_HEIGHT = 1080.0f;

inline float HUDScale(const ImVec2& screen)
{
    return std::min(screen.x / BASE_WIDTH, screen.y / BASE_HEIGHT);
}

inline ImVec2 S(ImVec2 v, float s) { return ImVec2(v.x * s, v.y * s); }
inline float  S(float v, float s)  { return v * s; }

float HUDScale_Match(const ImVec2& screen, float match)
{
    float scaleW = screen.x / BASE_WIDTH;
    float scaleH = screen.y / BASE_HEIGHT;

    // Unity behavior:
    return scaleW * (1.0f - match) + scaleH * match;
}

void DrawHalfLife2HUD3(const PlayerStatus& status)
{
    ImGuiIO& io = ImGui::GetIO();
    ImDrawList* draw = ImGui::GetForegroundDrawList();

    ImVec2 screen = io.DisplaySize;

    // ----------------------------------------------------------
    // Compute scale factor based on 1920x1080 reference
    // ----------------------------------------------------------
    float ui = HUDScale(screen);

    //float ui = HUDScale_Match(screen, 0.1f);  // equal weight like Unity default
    
    ui = std::max(ui, 0.6f);   

    // (optional) scale fonts globally
    ImGui::GetIO().FontGlobalScale = ui;

    // ----------------------------------------------------------
    // Scaled constants
    // ----------------------------------------------------------
    float padding      = S(20.0f, ui);
    float innerPadding = S(10.0f, ui);

    ImU32 hl2Orange = IM_COL32(255, 160, 0, 255);
    ImU32 hl2Blue   = IM_COL32(0, 170, 255, 255);
    ImU32 panelBg   = IM_COL32(0, 0, 0, 160);
    ImU32 barBg     = IM_COL32(0, 0, 0, 100);

    // --------------------------------------------------------------------
    // Bottom-left panel (Health + Suit)
    // --------------------------------------------------------------------
    ImVec2 panelSize = S(ImVec2(240, 80), ui);
    ImVec2 panelPos  = ImVec2(
        padding,
        screen.y - panelSize.y - padding
    );

    draw->AddRectFilled(panelPos,
        ImVec2(panelPos.x + panelSize.x, panelPos.y + panelSize.y),
        panelBg, S(6.0f, ui)
    );

    // ---------- HEALTH ----------
    float healthFrac = (float)status.health / 100.0f;

    ImVec2 healthPos     = ImVec2(panelPos.x + innerPadding, panelPos.y + innerPadding);
    ImVec2 healthBarSize = S(ImVec2(200, 16), ui);

    draw->AddRectFilled(
        healthPos,
        ImVec2(healthPos.x + healthBarSize.x, healthPos.y + healthBarSize.y),
        barBg, S(3.0f, ui)
    );

    draw->AddRectFilled(
        healthPos,
        ImVec2(healthPos.x + healthBarSize.x * healthFrac, healthPos.y + healthBarSize.y),
        hl2Orange, S(3.0f, ui)
    );

    draw->AddText(ImVec2(healthPos.x, healthPos.y - S(20.0f, ui)),
                  hl2Orange, "HEALTH");

    char healthText[16];
    sprintf(healthText, "%d", status.health);
    draw->AddText(ImVec2(healthPos.x + S(210.0f, ui), healthPos.y - S(2.0f, ui)),
                  hl2Orange, healthText);

    // ---------- SUIT ----------
    float armorFrac = (float)status.armor / 100.0f;

    ImVec2 armorPos = ImVec2(panelPos.x + innerPadding, panelPos.y + S(40.0f, ui));
    ImVec2 armorBarSize = S(ImVec2(200, 16), ui);

    draw->AddRectFilled(
        armorPos,
        ImVec2(armorPos.x + armorBarSize.x, armorPos.y + armorBarSize.y),
        barBg, S(3.0f, ui)
    );

    draw->AddRectFilled(
        armorPos,
        ImVec2(armorPos.x + armorBarSize.x * armorFrac, armorPos.y + armorBarSize.y),
        hl2Blue, S(3.0f, ui)
    );

    draw->AddText(ImVec2(armorPos.x, armorPos.y - S(20.0f, ui)),
                  hl2Blue, "SUIT");

    char armorText[16];
    sprintf(armorText, "%d", status.armor);
    draw->AddText(ImVec2(armorPos.x + S(210.0f, ui), armorPos.y - S(2.0f, ui)),
                  hl2Blue, armorText);

    // --------------------------------------------------------------------
    // Bottom-right panel (Ammo)
    // --------------------------------------------------------------------
    ImVec2 ammoPanelSize = S(ImVec2(180, 60), ui);
    ImVec2 ammoPanelPos = ImVec2(
        screen.x - ammoPanelSize.x - padding,
        screen.y - ammoPanelSize.y - padding
    );

    draw->AddRectFilled(
        ammoPanelPos,
        ImVec2(ammoPanelPos.x + ammoPanelSize.x, ammoPanelPos.y + ammoPanelSize.y),
        panelBg, S(6.0f, ui)
    );

    ImVec2 ammoInner = ImVec2(ammoPanelPos.x + innerPadding, ammoPanelPos.y + innerPadding);

    draw->AddText(ammoInner, hl2Orange, "AMMO");

    char ammoText[32];
    sprintf(ammoText, "%d / %d", status.ammo, status.ammoReserve);
    draw->AddText(ImVec2(ammoInner.x, ammoInner.y + S(25.0f, ui)), hl2Orange, ammoText);

    // --------------------------------------------------------------------
    // Crosshair (center)
    // --------------------------------------------------------------------
    ImVec2 center(screen.x * 0.5f, screen.y * 0.5f);
    float crossSize  = S(6.0f, ui);
    float thickness  = S(2.0f, ui);

    draw->AddLine(
        ImVec2(center.x - crossSize, center.y),
        ImVec2(center.x + crossSize, center.y),
        hl2Orange, thickness
    );

    draw->AddLine(
        ImVec2(center.x, center.y - crossSize),
        ImVec2(center.x, center.y + crossSize),
        hl2Orange, thickness
    );
}
*/

////////////////////////////

#include <algorithm>

//
// ─────────────────────────────────────────────────────────────
//   CONFIG
// ─────────────────────────────────────────────────────────────
//

static ImVec2 operator+(const ImVec2& a, const ImVec2& b){
    return {a.x + b.x, a.y + b.y};
}

/*static const float GUI_BASE_WIDTH  = 1920.0f;
static const float GUI_BASE_HEIGHT = 1080.0f;

enum class GUIAnchor
{
    TopLeft,
    TopCenter,
    TopRight,
    CenterLeft,
    Center,
    CenterRight,
    BottomLeft,
    BottomCenter,
    BottomRight
};

//
// ─────────────────────────────────────────────────────────────
//   SCALING (Unity-style Canvas Scaler)
// ─────────────────────────────────────────────────────────────
//

inline float GUI_GetScale(const ImVec2 &screen, float match = 0.5f, float minScale = 0.65f)
{
    float scaleW = screen.x / GUI_BASE_WIDTH;
    float scaleH = screen.y / GUI_BASE_HEIGHT;

    // Unity formula: Lerp(widthScale, heightScale, match)
    float s = scaleW * (1.0f - match) + scaleH * match;

    return std::max(s, minScale); // prevent unreadable UI
}

//
// ─────────────────────────────────────────────────────────────
//   Helper (scales vector/sizes)
// ─────────────────────────────────────────────────────────────
//

inline ImVec2 GUI_S(const ImVec2& v, float s){ return ImVec2(v.x*s, v.y*s); }
inline float  GUI_S(float v, float s){ return v*s; }

//
// ─────────────────────────────────────────────────────────────
//   Anchor to absolute screen position
// ─────────────────────────────────────────────────────────────
//

inline ImVec2 GUI_ApplyAnchor(GUIAnchor a, const ImVec2& screen, const ImVec2& size)
{
    switch(a)
    {
        case GUIAnchor::TopLeft:      return ImVec2(0, 0);
        case GUIAnchor::TopCenter:    return ImVec2(screen.x * 0.5f - size.x * 0.5f, 0);
        case GUIAnchor::TopRight:     return ImVec2(screen.x - size.x, 0);

        case GUIAnchor::CenterLeft:   return ImVec2(0, screen.y * 0.5f - size.y * 0.5f);
        case GUIAnchor::Center:       return ImVec2(screen.x*0.5f - size.x*0.5f, screen.y*0.5f - size.y*0.5f);
        case GUIAnchor::CenterRight:  return ImVec2(screen.x - size.x, screen.y * 0.5f - size.y * 0.5f);

        case GUIAnchor::BottomLeft:   return ImVec2(0, screen.y - size.y);
        case GUIAnchor::BottomCenter: return ImVec2(screen.x * 0.5f - size.x * 0.5f, screen.y - size.y);
        case GUIAnchor::BottomRight:  return ImVec2(screen.x - size.x, screen.y - size.y);
    }
    return ImVec2(0,0);
}

//
// ─────────────────────────────────────────────────────────────
//   Base UI Element
// ─────────────────────────────────────────────────────────────
//

struct GUIElement
{
    GUIAnchor anchor = GUIAnchor::TopLeft;

    ImVec2 position   = ImVec2(0,0); // offset in reference resolution
    ImVec2 size       = ImVec2(100,50);
    bool   visible    = true;

    float  rounding   = 6.0f;
    ImU32  bgColor    = IM_COL32(0,0,0,160);

    void DrawBackground(ImDrawList* dl, const ImVec2& screen, float scale)
    {
        if (!visible) return;

        ImVec2 scaledSize = GUI_S(size, scale);

        ImVec2 base = GUI_ApplyAnchor(anchor, screen, scaledSize);
        ImVec2 pos  = base + GUI_S(position, scale);

        dl->AddRectFilled(
            pos,
            ImVec2(pos.x + scaledSize.x, pos.y + scaledSize.y),
            bgColor,
            GUI_S(rounding, scale)
        );
    }
};


//
// ─────────────────────────────────────────────────────────────
//   Text helpers
// ─────────────────────────────────────────────────────────────
//

inline void GUI_DrawText(ImDrawList* dl, const char* text, ImVec2 pos, ImU32 col, float scale)
{
    pos = GUI_S(pos, scale);
    dl->AddText(pos, col, text);
}


//
// ─────────────────────────────────────────────────────────────
//   Bar element (Health, Armor etc.)
// ─────────────────────────────────────────────────────────────
//

struct GUIBar : public GUIElement
{
    float value = 1.0f;      // 0..1
    ImU32 fillColor = IM_COL32(255,255,255,255);
    float thickness = 0.0f;

    void Draw(ImDrawList* dl, const ImVec2& screen, float scale)
    {
        if (!visible) return;

        ImVec2 scaledSize = GUI_S(size, scale);
        ImVec2 base = GUI_ApplyAnchor(anchor, screen, scaledSize);
        ImVec2 pos  = base + GUI_S(position, scale);

        ImVec2 end  = ImVec2(pos.x + scaledSize.x, pos.y + scaledSize.y);

        // background
        dl->AddRectFilled(pos, end, bgColor, GUI_S(rounding, scale));

        // bar
        ImVec2 filledEnd(pos.x + scaledSize.x * value, end.y);
        dl->AddRectFilled(pos, filledEnd, fillColor, GUI_S(rounding, scale));
    }
};

void DrawExampleHUD()
{
    ImDrawList* dl = ImGui::GetForegroundDrawList();
    ImVec2 screen = ImGui::GetIO().DisplaySize;

    float scale = GUI_GetScale(screen);

    // Example #1 — Bottom-left health bar
    GUIBar health;
    health.anchor = GUIAnchor::BottomRight;
    health.position = {-30, -30}; //ImVec2(30, -150);     // offset from anchor
    health.size = ImVec2(300, 30);          // base resolution size
    health.value = 0.75f;
    health.fillColor = IM_COL32(255,140,0,255);
    health.Draw(dl, screen, scale);

    // Example #2 — Center text
    GUI_DrawText(
        dl, "CENTER MESSAGE", 
        ImVec2(screen.x*0.5f - 80, screen.y*0.5f), 
        IM_COL32(255,255,255,255), scale
    );
}*/


//======================================================
//  CONFIG: YOUR BASE REFERENCE RESOLUTION
//======================================================
static ImVec2 GUI_BASE_RESOLUTION(1920, 1080);

//======================================================
//  SCALE UTILITIES
//======================================================
static inline float GUI_ScaleX(float x, const ImVec2& screen)
{
    return x * (screen.x / GUI_BASE_RESOLUTION.x);
}

static inline float GUI_ScaleY(float y, const ImVec2& screen)
{
    return y * (screen.y / GUI_BASE_RESOLUTION.y);
}

float GUI_ComputeImGuiScale(const ImVec2& screen)
{
    float sx = screen.x / GUI_BASE_RESOLUTION.x;
    float sy = screen.y / GUI_BASE_RESOLUTION.y;
    return (sx + sy) * 0.5f;

    //float scale = std::min(sx, sy); Use the smaller value (preserve fit)
    //float scale = std::max(sx, sy); Use the bigger value (preserve readability)
}

static inline ImVec2 GUI_Scale(const ImVec2& v, const ImVec2& screen)
{
    //return ImVec2(GUI_ScaleX(v.x, screen), GUI_ScaleY(v.y, screen));

    float scale = GUI_ComputeImGuiScale(screen);
    return ImVec2(v.x * scale, v.y * scale);
}


//======================================================
//  ANCHORS
//======================================================
/*enum class GUIAnchor
{
    TopLeft,
    TopRight,
    BottomLeft,
    BottomRight,
    Center,
    CenterLeft,
    CenterRight
};*/

struct GUIAnchor
{
    float ax;   // 0 = left, 0.5 = center, 1 = right
    float ay;   // 0 = top,  0.5 = center, 1 = bottom

    constexpr GUIAnchor(float x = 0, float y = 0) : ax(x), ay(y) {}

    // presets
    static GUIAnchor TopLeft()      { return {0.0f, 0.0f}; }
    static GUIAnchor TopRight()     { return {1.0f, 0.0f}; }
    static GUIAnchor BottomLeft()   { return {0.0f, 1.0f}; }
    static GUIAnchor BottomRight()  { return {1.0f, 1.0f}; }
    static GUIAnchor Center()       { return {0.5f, 0.5f}; }
    static GUIAnchor CenterLeft()   { return {0.0f, 0.5f}; }
    static GUIAnchor CenterRight()  { return {1.0f, 0.5f}; }
};


static ImVec2 GUI_ApplyAnchor(ImVec2 pos, ImVec2 size, const GUIAnchor& a, const ImVec2& screen)
{
    // anchorX moves reference position along screen width
    pos.x = screen.x * a.ax - size.x * a.ax + pos.x;

    // anchorY moves reference position along screen height
    pos.y = screen.y * a.ay - size.y * a.ay + pos.y;

    return pos;
}


/*static ImVec2 GUI_ApplyAnchor(ImVec2 pos, ImVec2 size, GUIAnchor anchor, const ImVec2& screen)
{
    switch (anchor)
    {
    case GUIAnchor::Center:
        pos.x = screen.x * 0.5f - size.x * 0.5f;
        pos.y = screen.y * 0.5f - size.y * 0.5f;
        break;

    case GUIAnchor::TopRight:
        pos.x = screen.x - size.x - pos.x;
        break;

    case GUIAnchor::BottomLeft:
        pos.y = screen.y - size.y - pos.y;
        break;

    case GUIAnchor::BottomRight:
        pos.x = screen.x - size.x - pos.x;
        pos.y = screen.y - size.y - pos.y;
        break;

    
    //---------------------------------
    // NEW ANCHORS
    //---------------------------------
    case GUIAnchor::CenterLeft:
        pos.y = screen.y * 0.5f - size.y * 0.5f;
        // pos.x is unchanged (left edge)
        break;

    case GUIAnchor::CenterRight:
        pos.x = screen.x - size.x - pos.x;
        pos.y = screen.y * 0.5f - size.y * 0.5f;
        break;

    default:
        // TopLeft → do nothing
        break;

    }
    
    return pos;
}*/

//======================================================
// BASE GUI ELEMENT
//======================================================
/*struct GUIElement
{
    ImVec2 position;     // position in BASE resolution units
    ImVec2 size;         // size in BASE resolution units
    GUIAnchor anchor = GUIAnchor::TopLeft;

    bool visible = true;

    GUIElement(ImVec2 pos, ImVec2 size, GUIAnchor anchor = GUIAnchor::TopLeft)
        : position(pos), size(size), anchor(anchor) {}

    virtual ~GUIElement() {}

    virtual void Draw(ImDrawList* list, const ImVec2& screen) = 0;

protected:

    ImVec2 CalcFinalPos(const ImVec2& screen)
    {
        ImVec2 scaledPos = GUI_Scale(position, screen);
        ImVec2 scaledSize = GUI_Scale(size, screen);
        return GUI_ApplyAnchor(scaledPos, scaledSize, anchor, screen);
    }
};*/

struct GUIElement
{
    ImVec2 position;     // base resolution units
    ImVec2 size;         // base resolution units
    GUIAnchor anchor = GUIAnchor::TopLeft();
    bool visible = true;

    // NEW: hierarchical UI
    GUIElement* parent = nullptr;
    std::vector<GUIElement*> children;

    GUIElement() {}
    GUIElement(ImVec2 pos, ImVec2 size, GUIAnchor anch = GUIAnchor::TopLeft())
        : position(pos), size(size), anchor(anch) {}

    virtual ~GUIElement() {}

    //----------------------------------
    // Add child
    //----------------------------------
    void AddChild(GUIElement* child)
    {
        child->parent = this; //shared_from_this();
        children.push_back(child);
    }

    //----------------------------------
    // Calculate final position
    // If has parent: relative to parent
    // Otherwise: relative to screen
    //----------------------------------
    ImVec2 CalcFinalPos(const ImVec2& screen)
    {
        // Convert our position to pixel units
        ImVec2 scaledPos = GUI_Scale(position, screen);
        ImVec2 scaledSize = GUI_Scale(size, screen);

        // Check if has parent
        if (parent != nullptr) //auto p =  parent.lock())
        {
            // parent pixel position
            ImVec2 pFinal = parent->CalcFinalPos(screen);

            // parent pixel size
            ImVec2 pSizePx = GUI_Scale(parent->size, screen);

            // anchor relative to parent local rectangle
            ImVec2 anchored = GUI_ApplyAnchor(pFinal + scaledPos,
                                              scaledSize,
                                              anchor,
                                              pSizePx);    // parent area
            return anchored;
        }
        else
        {
            // Root element — use the screen size as reference
            ImVec2 anchored = GUI_ApplyAnchor(scaledPos,
                                              scaledSize,
                                              anchor,
                                              screen);
            return anchored;
        }
    }

    //----------------------------------
    // Main draw — base class draws nothing but calls children
    //----------------------------------
    virtual void Draw(ImDrawList* list, const ImVec2& screen)
    {
        if (!visible) return;

        // Draw children
        for (auto& c : children)
            if (c) c->Draw(list, screen);
    }
};

//======================================================
//  GUI TEXT ELEMENT (NEW!)
//======================================================
/*struct GUIText : public GUIElement
{
    std::string text;
    ImU32 color = IM_COL32(255, 255, 255, 255);
    float fontSize = 24.0f; // base size

    GUIText(const std::string& txt,
            float fontSize,
            ImVec2 pos,
            GUIAnchor anchor = GUIAnchor::TopLeft,
            ImU32 color = IM_COL32(255, 255, 255, 255))
        : GUIElement(pos, ImVec2(0, 0), anchor),
          text(txt), color(color), fontSize(fontSize) {}

    virtual void Draw(ImDrawList* list, const ImVec2& screen) override
    {
        if (!visible) return;

        ImVec2 pos = CalcFinalPos(screen);
        float scaledFont = GUI_ScaleY(fontSize, screen);

        list->AddText(nullptr, scaledFont, pos, color, text.c_str());
    }
};*/

// -----------------------------
static inline ImVec2 GUI_Unscale(const ImVec2& scaledPixels, const ImVec2& screen)
{
    return ImVec2(
        scaledPixels.x * (GUI_BASE_RESOLUTION.x / screen.x),
        scaledPixels.y * (GUI_BASE_RESOLUTION.y / screen.y)
    );
}

struct GUIText : public GUIElement
{
    std::string text;
    ImU32 color = IM_COL32(255,255,255,255);
    float fontSize = 24.0f; // base size (units in BASE resolution)
    float wrapWidth = 0.0f; // 0 = no wrap

    GUIText(const std::string& txt,
            float fontSizeBase,
            ImVec2 pos,
            GUIAnchor anchor = GUIAnchor::TopLeft(),
            ImU32 color = IM_COL32(255,255,255,255))
        : GUIElement(pos, ImVec2(0,0), anchor),
          text(txt),
          color(color),
          fontSize(fontSizeBase),
          wrapWidth(0.0f)
    {}

    virtual void Draw(ImDrawList* list, const ImVec2& screen) override
    {
        /*if (!visible) return;

        // 1) compute scaled font size in pixels (scale by Y to keep aspect)
        float scaledFontPx = GUI_ScaleY(fontSize, screen);

        // 2) measure text size (in pixels) using ImFont (respects scaledFontPx)
        ImFont* font = ImGui::GetFont(); // current font
        // Use ImFont::CalcTextSizeA to measure precisely at the wanted pixel size
        ImVec2 textSizePx = font->CalcTextSizeA(
            scaledFontPx,
            (wrapWidth > 0.0f) ? GUI_ScaleX(wrapWidth, screen) : FLT_MAX,
            0.0f,
            text.c_str()
        );

        // 3) update this element size (store in BASE resolution units)
        // Convert measured pixel size back into base-resolution units so the element's
        // size member remains expressed in the same base units as other elements.
        this->size = GUI_Unscale(textSizePx, screen);

        // 4) compute final position using the element base-position + anchor
        // CalcFinalPos() uses GUI_Scale(position, screen) and GUI_Scale(size, screen)
        ImVec2 scaledPos = GUI_Scale(position, screen);
        ImVec2 scaledSize = textSizePx; // we already have size in pixels
        ImVec2 finalPos = GUI_ApplyAnchor(scaledPos, scaledSize, anchor, screen);

        // 5) submit text using the measured font size
        // Use ImDrawList::AddText overload that takes font & size for consistent measurement
        list->AddText(font, scaledFontPx, finalPos, color, text.c_str());
        */

        if (!visible) return;

        float scaledFontPx = GUI_ScaleY(fontSize, screen);

        ImFont* font = ImGui::GetFont();
        ImVec2 textSizePx = font->CalcTextSizeA(
            scaledFontPx, 
            FLT_MAX, 
            0.0f, 
            text.c_str()
        );

        // element size stored in BASE resolution units
        size = GUI_Unscale(textSizePx, screen);

        // NEW — parent-aware position
        ImVec2 finalPos = CalcFinalPos(screen);

        // draw text
        list->AddText(font, scaledFontPx, finalPos, color, text.c_str());

        // draw children (if any)
        GUIElement::Draw(list, screen);

    }
};

//======================================================
//  GUI BOX RECT
//======================================================
struct GUIBox : public GUIElement
{
    ImU32 color;
    float rounding;

    GUIBox(ImVec2 pos, ImVec2 size, GUIAnchor anchor, ImU32 color, float rounding = 6.0f)
        : GUIElement(pos, size, anchor), color(color), rounding(rounding) {}

    virtual void Draw(ImDrawList* list, const ImVec2& screen) override{
        if(!visible) return;

        ImVec2 pos = CalcFinalPos(screen);
        ImVec2 s = GUI_Scale(size, screen);

        list->AddRectFilled(
            pos, ImVec2(pos.x + s.x, pos.y + s.y),
            color, rounding
        );

        GUIElement::Draw(list, screen);                           
    }
};

//======================================================
//  GUI BAR (HEALTH, AMMO, ETC.)
//======================================================
struct GUIBar : public GUIElement
{
    float fraction = 1.0f;
    ImU32 bgColor;
    ImU32 fillColor;
    float rounding;

    GUIBar(ImVec2 pos, ImVec2 size, GUIAnchor anchor,
           ImU32 bgColor, ImU32 fillColor)
        : GUIElement(pos, size, anchor),
          bgColor(bgColor), fillColor(fillColor),
          rounding(3.0f) {}

    virtual void Draw(ImDrawList* list, const ImVec2& screen) override
    {
        if (!visible) return;

        ImVec2 pos = CalcFinalPos(screen);
        ImVec2 s = GUI_Scale(size, screen);

        // Background
        list->AddRectFilled(pos, ImVec2(pos.x + s.x, pos.y + s.y),
                            bgColor, rounding);

        // Fill
        list->AddRectFilled(pos, ImVec2(pos.x + s.x * fraction, pos.y + s.y),
                            fillColor, rounding);
    }
};

static ImVec2 GUI_FixOffsetForAnchor(const ImVec2& pos, const GUIAnchor& anchor)
{
    ImVec2 result = pos;

    // If anchor.x is on the RIGHT ( > 0.5 ), invert X offset
    if (anchor.ax > 0.5f)
        result.x = -pos.x;

    // If anchor.y is on the BOTTOM ( > 0.5 ), invert Y offset
    if (anchor.ay > 0.5f)
        result.y = -pos.y;

    return result;
}

static ImVec2 GUI_FixOffsetForAnchor2(const ImVec2& pos, const GUIAnchor& a)
{
    // Smooth continuous mapping:
    // anchor = 0   → +1
    // anchor = 0.5 → 0
    // anchor = 1   → -1
    float sx = (0.5f - a.ax) * 2.0f;
    float sy = (0.5f - a.ay) * 2.0f;

    return ImVec2(pos.x * sx, pos.y * sy);
}

class GUIImguiWindow : public GUIElement
{
public:
    std::string name;
    ImGuiWindowFlags flags = ImGuiWindowFlags_None;

    GUIImguiWindow(const std::string& winName,
                   ImVec2 pos,
                   ImVec2 size,
                   GUIAnchor anchor
    ):GUIElement(pos, size, anchor), name(winName)
    {
        //name = winName;
        /*this->position = localPos;
        this->size = size;
        this->anchor = anchor;*/
    }

    std::function<void()> drawContents;

    /*void Draw(ImDrawList* list, const ImVec2& screen) override{
        ImVec2 pos = CalcFinalPos(screen);
        ImVec2 s = GUI_Scale(size, screen);

        ImGui::SetNextWindowPos(pos);
        ImGui::SetNextWindowSize(s);

        ImGui::Begin(name.c_str(), nullptr, flags);
        if(drawContents) drawContents();
        ImGui::End();
    }*/

    void Draw(ImDrawList* list, const ImVec2& screen)
    {
        ImVec2 pos = CalcFinalPos(screen);
        ImVec2 s   = GUI_Scale(size, screen);

        // Our custom scale factor based on screen size
        float scale = GUI_ComputeImGuiScale(screen); //CalcScale(screen);

        ImGuiStyle& style = ImGui::GetStyle();

        // Backup style
        ImGuiStyle backup = style;

        // Apply window-only scaling
        style.ScaleAllSizes(scale);
        //ImGui::GetIO().FontGlobalScale = scale * 2;
        customFont->Scale = scale * 2;
        ImGui::PushFont(customFont);

        ImGui::SetNextWindowPos(pos);
        ImGui::SetNextWindowSize(s);

        ImGui::Begin(name.c_str(), nullptr, flags);

        if(drawContents)
            drawContents();  // default ImGui widgets now appear scaled

        ImGui::End();

        // Restore style so other windows aren't affected
        style = backup;
        //ImGui::GetIO().FontGlobalScale = 1.0f;
        ImGui::PopFont();
    }

};

void DrawGameHUD(const PlayerStatus& status)
{
    ImDrawList* dl = ImGui::GetForegroundDrawList();
    ImVec2 screen = ImGui::GetIO().DisplaySize;

    /*// Panel background
    GUIBox panel(ImVec2(0, 0), ImVec2(300, 100), GUIAnchor::BottomLeft, IM_COL32(0,0,0,160));
    panel.Draw(dl, screen);

    // Health bar
    GUIBar health({30, 30}, ImVec2(30, 30), GUIAnchor::BottomLeft,
                  IM_COL32(0,0,0,100), IM_COL32(255,160,0,255));
    health.fraction = status.health / 100.f;
    health.Draw(dl, screen);

    // Health label
    GUIText tHealth("HEALTH", 20, ImVec2(30, 60), GUIAnchor::BottomLeft, IM_COL32(255,160,0,255));
    tHealth.Draw(dl, screen);

    // Ammo text anchored bottom-right
    GUIText ammoText(
        std::to_string(status.ammo) + " / " + std::to_string(status.ammoReserve),
        26,
        ImVec2(30, 30),
        GUIAnchor::BottomRight,
        IM_COL32(255,160,0,255)
    );
    ammoText.Draw(dl, screen);*/

    auto pos = ImVec2(30, 30);   // always positive
    auto anchor = GUIAnchor(1,0.5f); // bottom-right
    pos = GUI_FixOffsetForAnchor2(pos, anchor);
    GUIBox panel(pos, ImVec2(300, 100), anchor, IM_COL32(0,0,0,160));
    
    // Health label
    GUIText tHealth("HEALTH", 30, ImVec2(30, 10), GUIAnchor::TopLeft(), IM_COL32(255,160,0,255));

    // Health bar
    GUIBar health({30, 50}, ImVec2(300 - 30, 30), GUIAnchor::TopLeft(), IM_COL32(0,0,0,100), IM_COL32(255,160,0,255));
    health.fraction = status.health / 100.f;

    panel.AddChild(&tHealth);
    panel.AddChild(&health);
    panel.Draw(dl, screen);

    auto inventoryWin = std::make_shared<GUIImguiWindow>(
        "Inventory",
        ImVec2(-30, 30),         // base offset
        ImVec2(320, 400),       // base size
        GUIAnchor(1, 0.0f)      // Top-Right anchor (x=1, y=0)
    );
    /*inventoryWin->drawContents = [&](){
        ImGui::Text("Weapons");
        ImGui::Separator();
        ImGui::Button("Shotgun");
        ImGui::Button("Crowbar");
    };*/
    inventoryWin->drawContents = [&](){
        ImGui::Text("Weapons");
        ImGui::Separator();

        if(ImGui::CollapsingHeader("Primary Weapons", ImGuiTreeNodeFlags_DefaultOpen)){
            ImGui::Selectable("Shotgun", false);
            ImGui::Selectable("Assault Rifle", false);
            ImGui::Selectable("Sniper Rifle", false);

            ImGui::Spacing();
            ImGui::TextDisabled("Ammo Counts:");
            ImGui::BulletText("Shotgun: %d / %d", 6, 24);
            ImGui::BulletText("AR: %d / %d", 30, 180);
            ImGui::BulletText("Sniper: %d / %d", 1, 20);
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        if(ImGui::CollapsingHeader("Melee Weapons", ImGuiTreeNodeFlags_DefaultOpen)){
            ImGui::Selectable("Crowbar", false);
            ImGui::Selectable("Katana", false);
            ImGui::Selectable("Combat Knife", false);
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        ImGui::Text("Inventory:");
        ImGui::Columns(2, "inventoryCols", false);

        ImGui::Text("Item"); ImGui::NextColumn();
        ImGui::Text("Quantity"); ImGui::NextColumn();
        ImGui::Separator();

        ImGui::Text("Medkit");   ImGui::NextColumn(); ImGui::Text("3"); ImGui::NextColumn();
        ImGui::Text("Bandages"); ImGui::NextColumn(); ImGui::Text("12"); ImGui::NextColumn();
        ImGui::Text("Battery");  ImGui::NextColumn(); ImGui::Text("5"); ImGui::NextColumn();
        ImGui::Text("Food Can"); ImGui::NextColumn(); ImGui::Text("8"); ImGui::NextColumn();

        ImGui::Columns(1);

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        ImGui::Text("Actions:");
        if(ImGui::Button("Use Item", ImVec2(-ImGui::GetContentRegionAvail().x * 0.5f, 0))){
            // test button
        }

        ImGui::SameLine();

        if(ImGui::Button("Drop Item", ImVec2(-1, 0))){
            // test button
        }
    };

    inventoryWin->Draw(dl, screen);
}

void BaseMeshSample::OnGUI(){
    OD::ImGuiLayer::SetCleanAll(false);
    //static bool show;
    //ImGui::ShowDemoWindow(&show);

    bool a = false;
    bool b = false;
    bool c = false;
    //DrawMainMenu(a, b, c);

    PlayerStatus status;
    status.health = 87;
    status.armor = 56;
    status.ammo = 30;
    status.ammoReserve = 120;

    /*DrawHalfLife2HUD2(status);*/

    /*RenderHUD();*/

    //DrawExampleHUD();

    DrawGameHUD(status);

    ImGui::Text("Test");
}

void BaseMeshSample::OnResize(int width, int height){}
void BaseMeshSample::OnExit(){}