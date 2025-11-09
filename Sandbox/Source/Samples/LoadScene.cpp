#include "LoadScene.h"
#include "Standard/Module.h"
#include <OD/Core/Application.h>
#include <OD/Scene/SceneManager.h>
#include <OD/Editor/Editor.h>

void LoadSceneSample::OnInit(){
    LogInfo("Game Init");
    Application::Vsync(false);

    Standard::ModuleInit();

    auto& SceneManager = SceneManager::Get();
    OD::Scene* scene = SceneManager.NewScene();

    scene->Load("Standard/Scenes/Prototype.scene");
    //scene->Load("Sandbox/Scenes/TerrainTest2.scene");

    Application::AddModule<Editor>();
    //scene->Start();
}

void LoadSceneSample::OnUpdate(float deltaTime){}   
void LoadSceneSample::OnRender(float deltaTime){}


#include <vector>
#include <string>
#include <algorithm>  // For std::min

// Helper functions for ImVec2 operations
ImVec2 operator+(const ImVec2& a, const ImVec2& b) {
    return ImVec2(a.x + b.x, a.y + b.y);
}

ImVec2 operator+(const ImVec2& a, float b) {
    return ImVec2(a.x + b, a.y + b);
}

ImVec2 operator-(const ImVec2& a, const ImVec2& b) {
    return ImVec2(a.x - b.x, a.y - b.y);
}

ImVec2 operator-(const ImVec2& a, float b) {
    return ImVec2(a.x - b, a.y - b);
}

ImVec2 operator*(const ImVec2& a, float scalar) {
    return ImVec2(a.x * scalar, a.y * scalar);
}

ImVec2 operator/(const ImVec2& a, float scalar) {
    return ImVec2(a.x / scalar, a.y / scalar);
}

#include "OD/Core/ImGui.h"

#include "imgui.h"

// Example layer names
static const char* layerNames[] = {
    "Default", "TransparentFX", "Ignore Raycast", "Layer3", "Water", "UI", "Layer6", "Humans"
};
static constexpr int layerCount = IM_ARRAYSIZE(layerNames);

// Matrix of booleans to store which layer collides with which
static bool collisionMatrix[layerCount][layerCount] = {};

void DrawLayerCollisionMatrix(){
    const float TEXT_BASE_HEIGHT = ImGui::GetTextLineHeightWithSpacing();
    ImGuiTableFlags tableFlags = ImGuiTableFlags_SizingFixedFit
        | ImGuiTableFlags_BordersInnerV
        | ImGuiTableFlags_BordersOuter
        | ImGuiTableFlags_ScrollY
        | ImGuiTableFlags_ScrollX;

    ImGui::GetStyle().TableAngledHeadersAngle = 0; //-45.0f;
    ImGui::GetStyle().TableAngledHeadersTextAlign = ImVec2(0.5f, 0.5f);

    if(ImGui::BeginTable(
        "LayerCollisionMatrix", layerCount + 1, tableFlags//,
        //ImVec2(0, TEXT_BASE_HEIGHT * (layerCount + 2))
    )){
        // Set up angled headers
        ImGui::TableSetupColumn("Layer", ImGuiTableColumnFlags_NoReorder | ImGuiTableColumnFlags_NoHide);
        for (int i = 0; i < layerCount; i++)
            ImGui::TableSetupColumn(layerNames[i], ImGuiTableColumnFlags_AngledHeader | ImGuiTableColumnFlags_WidthFixed);

        // Draw angled headers
        ImGui::TableAngledHeadersRow();

        // Draw each layer row
        for (int row = 0; row < layerCount; row++)
        {
            ImGui::PushID(row);
            ImGui::TableNextRow();
            for (int col = 0; col < layerCount + 1; col++){
                ImGui::TableSetColumnIndex(col);

                // First column: layer name
                if(col == 0){
                    ImGui::AlignTextToFramePadding();
                    ImGui::TextUnformatted(layerNames[row]);
                } else {
                    // Avoid redundant duplicate matrix cells (only fill upper-right)
                    if(col - 1 < row)
                        continue;

                    ImGui::PushID(col);
                    bool& val = collisionMatrix[row][col - 1];
                    ImGui::Checkbox("", &val);

                    // Mirror value to keep matrix symmetrical
                    if (row != col - 1)
                        collisionMatrix[col - 1][row] = val;
                    ImGui::PopID();
                }
            }
            ImGui::PopID();
        }

        ImGui::EndTable();
    }
}

void DrawLayerCollisionMatrix2(){
    const float TEXT_BASE_HEIGHT = ImGui::GetTextLineHeightWithSpacing();
    ImGuiTableFlags tableFlags = ImGuiTableFlags_SizingFixedFit
        | ImGuiTableFlags_BordersInnerV
        | ImGuiTableFlags_BordersOuter
        | ImGuiTableFlags_ScrollY
        | ImGuiTableFlags_ScrollX;

    // Straight headers (no rotation)
    ImGui::GetStyle().TableAngledHeadersAngle = 0;
    ImGui::GetStyle().TableAngledHeadersTextAlign = ImVec2(0.5f, 0.5f);

    if (ImGui::BeginTable("LayerCollisionMatrix", layerCount + 1, tableFlags)) {
        // First column: row names
        ImGui::TableSetupColumn("Layer", ImGuiTableColumnFlags_NoReorder | ImGuiTableColumnFlags_NoHide);

        // 🔁 Reversed order for angled headers
        for (int i = layerCount - 1; i >= 0; --i)
            ImGui::TableSetupColumn(layerNames[i], ImGuiTableColumnFlags_AngledHeader | ImGuiTableColumnFlags_WidthFixed);

        ImGui::TableAngledHeadersRow();

        // Draw each layer row (same as before)
        for (int row = 0; row < layerCount; row++) {
            ImGui::PushID(row);
            ImGui::TableNextRow();

            for (int col = 0; col < layerCount + 1; col++) {
                ImGui::TableSetColumnIndex(col);

                if (col == 0) {
                    // Left labels
                    ImGui::AlignTextToFramePadding();
                    ImGui::TextUnformatted(layerNames[row]);
                } else {
                    // Reversed column index mapping
                    int realCol = (layerCount - col);

                    // Skip upper-left half → only triangle (same condition)
                    if (realCol < row)
                        continue;

                    ImGui::PushID(col);
                    bool& val = collisionMatrix[row][realCol];
                    ImGui::Checkbox("", &val);

                    // Mirror across diagonal
                    if (row != realCol)
                        collisionMatrix[realCol][row] = val;
                    ImGui::PopID();
                }
            }
            ImGui::PopID();
        }

        ImGui::EndTable();
    }
}

void DrawLayerCollisionMatrix3(){
    const float TEXT_BASE_HEIGHT = ImGui::GetTextLineHeightWithSpacing();
    ImGuiTableFlags tableFlags = ImGuiTableFlags_SizingFixedFit
        | ImGuiTableFlags_BordersInnerV
        | ImGuiTableFlags_BordersOuter
        | ImGuiTableFlags_ScrollY
        | ImGuiTableFlags_ScrollX;

    ImGuiStyle& style = ImGui::GetStyle();
    style.TableAngledHeadersAngle = 0;
    style.TableAngledHeadersTextAlign = ImVec2(0.5f, 0.5f);

    // 🧩 Collect valid layers (skip default-named ones)
    std::vector<int> visibleLayers;
    for (int i = 0; i < layerCount; ++i)
    {
        std::string defaultName = "Layer" + std::to_string(i);
        if (strcmp(layerNames[i], defaultName.c_str()) != 0 && layerNames[i][0] != '\0')
            visibleLayers.push_back(i);
    }

    if (visibleLayers.empty())
        return; // Nothing to draw

    int visibleCount = (int)visibleLayers.size();

    if (ImGui::BeginTable("LayerCollisionMatrix", visibleCount + 1, tableFlags)) {
        ImGui::TableSetupColumn("Layer", ImGuiTableColumnFlags_NoReorder | ImGuiTableColumnFlags_NoHide);

        // 🔁 Reversed order (for angled headers)
        for (int idx = visibleCount - 1; idx >= 0; --idx)
        {
            int layerIndex = visibleLayers[idx];
            ImGui::TableSetupColumn(layerNames[layerIndex],
                ImGuiTableColumnFlags_AngledHeader | ImGuiTableColumnFlags_WidthFixed);
        }

        ImGui::TableAngledHeadersRow();

        // Draw visible layer rows
        for (int r = 0; r < visibleCount; ++r)
        {
            int row = visibleLayers[r];
            ImGui::PushID(row);
            ImGui::TableNextRow();

            for (int c = 0; c < visibleCount + 1; ++c)
            {
                ImGui::TableSetColumnIndex(c);

                if (c == 0)
                {
                    ImGui::AlignTextToFramePadding();
                    ImGui::TextUnformatted(layerNames[row]);
                }
                else
                {
                    int realCol = visibleLayers[visibleCount - c];

                    if (realCol < row)
                        continue;

                    ImGui::PushID(c);
                    bool& val = collisionMatrix[row][realCol];
                    ImGui::Checkbox("", &val);
                    if (row != realCol)
                        collisionMatrix[realCol][row] = val;
                    ImGui::PopID();
                }
            }

            ImGui::PopID();
        }

        ImGui::EndTable();
    }
}

void LoadSceneSample::OnGUI(){
    //DrawLayerCollisionMatrix3();

    //static bool show;
    //ImGui::ShowDemoWindow(&show);


    /*const float TEXT_BASE_WIDTH = ImGui::CalcTextSize("A").x;
    const float TEXT_BASE_HEIGHT = ImGui::GetTextLineHeightWithSpacing();

    if (ImGui::TreeNode("Angled headers")){
        const char* column_names[] = { "Track", "cabasa", "ride", "smash", "tom-hi", "tom-mid", "tom-low", "hihat-o", "hihat-c", "snare-s", "snare-c", "clap", "rim", "kick" };
        const int columns_count = IM_ARRAYSIZE(column_names);
        const int rows_count = 12;

        static ImGuiTableFlags table_flags = ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_ScrollX | ImGuiTableFlags_ScrollY | ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersInnerH | ImGuiTableFlags_Hideable | ImGuiTableFlags_Resizable | ImGuiTableFlags_Reorderable | ImGuiTableFlags_HighlightHoveredColumn;
        static ImGuiTableColumnFlags column_flags = ImGuiTableColumnFlags_AngledHeader | ImGuiTableColumnFlags_WidthFixed;
        static bool bools[columns_count * rows_count] = {}; // Dummy storage selection storage
        static int frozen_cols = 1;
        static int frozen_rows = 2;
        ImGui::CheckboxFlags("_ScrollX", &table_flags, ImGuiTableFlags_ScrollX);
        ImGui::CheckboxFlags("_ScrollY", &table_flags, ImGuiTableFlags_ScrollY);
        ImGui::CheckboxFlags("_Resizable", &table_flags, ImGuiTableFlags_Resizable);
        ImGui::CheckboxFlags("_NoBordersInBody", &table_flags, ImGuiTableFlags_NoBordersInBody);
        ImGui::CheckboxFlags("_HighlightHoveredColumn", &table_flags, ImGuiTableFlags_HighlightHoveredColumn);
        ImGui::SetNextItemWidth(ImGui::GetFontSize() * 8);
        ImGui::SliderInt("Frozen columns", &frozen_cols, 0, 2);
        ImGui::SetNextItemWidth(ImGui::GetFontSize() * 8);
        ImGui::SliderInt("Frozen rows", &frozen_rows, 0, 2);
        ImGui::CheckboxFlags("Disable header contributing to column width", &column_flags, ImGuiTableColumnFlags_NoHeaderWidth);

        if (ImGui::TreeNode("Style settings"))
        {
            ImGui::SameLine();
            //HelpMarker("Giving access to some ImGuiStyle value in this demo for convenience.");
            ImGui::SetNextItemWidth(ImGui::GetFontSize() * 8);
            ImGui::SliderAngle("style.TableAngledHeadersAngle", &ImGui::GetStyle().TableAngledHeadersAngle, -50.0f, +50.0f);
            ImGui::SetNextItemWidth(ImGui::GetFontSize() * 8);
            ImGui::SliderFloat2("style.TableAngledHeadersTextAlign", (float*)&ImGui::GetStyle().TableAngledHeadersTextAlign, 0.0f, 1.0f, "%.2f");
            ImGui::TreePop();
        }

        if (ImGui::BeginTable("table_angled_headers", columns_count, table_flags, ImVec2(0.0f, TEXT_BASE_HEIGHT * 12)))
        {
            ImGui::TableSetupColumn(column_names[0], ImGuiTableColumnFlags_NoHide | ImGuiTableColumnFlags_NoReorder);
            for (int n = 1; n < columns_count; n++)
                ImGui::TableSetupColumn(column_names[n], column_flags);
            ImGui::TableSetupScrollFreeze(frozen_cols, frozen_rows);

            ImGui::TableAngledHeadersRow(); // Draw angled headers for all columns with the ImGuiTableColumnFlags_AngledHeader flag.
            ImGui::TableHeadersRow();       // Draw remaining headers and allow access to context-menu and other functions.
            for (int row = 0; row < rows_count; row++)
            {
                ImGui::PushID(row);
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::AlignTextToFramePadding();
                ImGui::Text("Track %d", row);
                for (int column = 1; column < columns_count; column++)
                    if (ImGui::TableSetColumnIndex(column))
                    {
                        ImGui::PushID(column);
                        ImGui::Checkbox("", &bools[row * columns_count + column]);
                        ImGui::PopID();
                    }
                ImGui::PopID();
            }
            ImGui::EndTable();
        }
        ImGui::TreePop();
    }*/

}

void LoadSceneSample::OnResize(int width, int height){}
void LoadSceneSample::OnExit(){}