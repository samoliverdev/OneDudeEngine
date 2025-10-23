#include "BaseMesh.h"
#include "ECSTest.h"
#include <OD/Graphics/Font.h>
#include <OD/Graphics/Material.h>
#include <OD/Graphics/Graphics.h>
#include <OD/Scene/Scene.h>
#include <OD/Core/Input.h>
#include <OD/Core/Application.h>
#include <OD/Core/ImGui.h>
#include <OD/RenderPipeline/MeshRendererComponent.h>

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
        LogInfo("------Name: %s", name.name.c_str());
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
    DrawHalfLife2HUD2(status);
}

void BaseMeshSample::OnResize(int width, int height){}
void BaseMeshSample::OnExit(){}