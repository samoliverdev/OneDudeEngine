#include "OD/pch.h"
#include "LightComponent.h"
#include "OD/Serialization/CerealImGui.h"
#include "OD/Core/ImGui.h"
#include "OD/Core/Lua.h"
#include <imgui/imgui_internal.h>

namespace OD{

void LightComponent::OnGui(Entity& e, Scene& scene){
    LightComponent& light = scene.GetComponent<LightComponent>(e);

    /*cereal::ImGuiArchive uiArchive;
    //cereal::ImGuiArchive::Options colorOpt;
    //colorOpt.colorHDR = true;
    //uiArchive.setOption("color", colorOpt);
    uiArchive.setOption("intensity", cereal::ImGuiArchive::Options().setMinMax(0, 100000));
    uiArchive(light);
    return;*/

    /*const char* projectionTypeString[] = {"Directional", "Point", "Spot"};
    const char* curProjectionTypeString = projectionTypeString[(int)light.type];
    if(ImGui::BeginCombo("Type", curProjectionTypeString)){
        for(int i = 0; i < 3; i++){
            bool isSelected = curProjectionTypeString == projectionTypeString[i];
            if(ImGui::Selectable(projectionTypeString[i], isSelected)){
                curProjectionTypeString = projectionTypeString[i];
                light.type = (LightComponent::Type)i;
            }

            if(isSelected) ImGui::SetItemDefaultFocus();
            
        }

        ImGui::EndCombo();
    }*/

    ImGui::DrawEnumCombo<LightComponent::Type>("type", &light.type);

    ImGui::Spacing();ImGui::Spacing();

    /*float color[] = {light.color.r, light.color.g, light.color.b, light.color.a};
    if(ImGui::ColorEdit4("color", color)){
        light.color = Color{color[0], color[1], color[2], color[3]};
    }*/

    /*ImGui::ColorEdit4("color", &light.color);

    ImGui::DragFloat("intensity", &light.intensity, 0.025f, 0, 1000, "%.3f", ImGuiSliderFlags_AlwaysClamp);
    ImGui::DragFloat("specular", &light.specular, 0.025f, 0, 1, "%.3f", ImGuiSliderFlags_AlwaysClamp);
    ImGui::DragFloat("falloff", &light.falloff, 0.025f, 0, 1, "%.3f", ImGuiSliderFlags_AlwaysClamp);

    ImGui::Spacing();ImGui::Spacing();

    if(light.type == LightComponent::Type::Point){
        ImGui::DragFloat("radius", &light.radius, 0.1f, 0, 1000, "%.3f", ImGuiSliderFlags_AlwaysClamp);
    }

    if(light.type == LightComponent::Type::Spot){
        ImGui::DragFloat("radius", &light.radius, 0.1f, 0, 1000, "%.3f", ImGuiSliderFlags_AlwaysClamp);

        if(light.coneAngleInner > light.coneAngleOuter){
            light.coneAngleInner = light.coneAngleOuter;
        }

        ImGui::DragFloat("coneAngleInner", &light.coneAngleInner, 0.1f, 0, 1000, "%.3f", ImGuiSliderFlags_AlwaysClamp);
        ImGui::DragFloat("coneAngleOuter", &light.coneAngleOuter, 0.1f, 0, 1000, "%.3f", ImGuiSliderFlags_AlwaysClamp);
        
    }

    ImGui::Checkbox("renderShadow", &light.renderShadow);*/


    /*ImGui::BeginTable(
        "LightProperties", 2,
        ImGuiTableFlags_SizingStretchSame | 
        ImGuiTableFlags_NoPadOuterX | 
        ImGuiTableFlags_BordersInnerV
    );

    float availableWidth = ImGui::GetContentRegionAvail().x;
    float labelWidth = math::clamp(availableWidth * 0.25f, 50.0f, 100.0f);

    // Set column widths
    ImGui::TableSetupColumn("Property", ImGuiTableColumnFlags_WidthFixed, 90.0f);
    ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);

    // Generic function to add a table row with a custom control
    auto AddTableRow = [&](const char* label, const char* id, auto&& renderControl) {
        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::Text(label);
        ImGui::TableSetColumnIndex(1);
        ImGui::PushItemWidth(-1);
        renderControl(id);
        ImGui::PopItemWidth();
    };

    AddTableRow("Type", "##type", [&](const char* id) {
        ImGui::DrawEnumCombo<LightComponent::Type>(id, &light.type);
    });

    AddTableRow("Color", "##Color", [&](const char* id) {
        ImGui::ColorEdit4(id, &light.color);
    });

    // Intensity
    AddTableRow("Intensity", "##intensity", [&](const char* id) {
        ImGui::DragFloat(id, &light.intensity, 0.025f, 0, 1000, "%.3f", ImGuiSliderFlags_AlwaysClamp);
    });

    // Specular
    AddTableRow("Specular", "##specular", [&](const char* id) {
        ImGui::DragFloat(id, &light.specular, 0.025f, 0, 1, "%.3f", ImGuiSliderFlags_AlwaysClamp);
    });

    // Falloff
    AddTableRow("Falloff", "##falloff", [&](const char* id) {
        ImGui::DragFloat(id, &light.falloff, 0.025f, 0, 1, "%.3f", ImGuiSliderFlags_AlwaysClamp);
    });

    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    ImGui::Spacing(); ImGui::Spacing();

    if (light.type == LightComponent::Type::Point) {
        AddTableRow("Radius", "##radius", [&](const char* id) {
            ImGui::DragFloat(id, &light.radius, 0.1f, 0, 1000, "%.3f", ImGuiSliderFlags_AlwaysClamp);
        });
    }

    if (light.type == LightComponent::Type::Spot) {
        AddTableRow("Radius", "##radius_spot", [&](const char* id) {
            ImGui::DragFloat(id, &light.radius, 0.1f, 0, 1000, "%.3f", ImGuiSliderFlags_AlwaysClamp);
        });

        if (light.coneAngleInner > light.coneAngleOuter) {
            light.coneAngleInner = light.coneAngleOuter;
        }

        AddTableRow("Cone Angle Inner", "##coneAngleInner", [&](const char* id) {
            ImGui::DragFloat(id, &light.coneAngleInner, 0.1f, 0, 1000, "%.3f", ImGuiSliderFlags_AlwaysClamp);
        });

        AddTableRow("Cone Angle Outer", "##coneAngleOuter", [&](const char* id) {
            ImGui::DragFloat(id, &light.coneAngleOuter, 0.1f, 0, 1000, "%.3f", ImGuiSliderFlags_AlwaysClamp);
        });
    }

    // Checkbox row
    AddTableRow("Render Shadow", "##renderShadow", [&](const char* id) {
        ImGui::Checkbox(id, &light.renderShadow);
    });

    ImGui::EndTable();
    */

    ImGui::BeginTableEx(
        "CameraProperties", ImGui::GlobalTableID, 2,
        ImGuiTableFlags_NoSavedSettings |
        ImGuiTableFlags_Resizable |                    // Permite redimensionar colunas manualmente
        //ImGuiTableFlags_SizingStretchSame |            // Faz com que as colunas preencham o espaço igualmente
        ImGuiTableFlags_SizingStretchProp |
        ImGuiTableFlags_NoPadOuterX | 
        ImGuiTableFlags_BordersInnerV
    );

    // Configura as colunas com comportamento Unreal-like
    //ImGui::TableSetupColumn("Property", ImGuiTableColumnFlags_None, 90.0f);  // Não usar WidthFixed!
    //ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);    // Stretch permite expandir o restante
    ImGui::TableSetupColumn("Property", ImGuiTableColumnFlags_WidthStretch, 1.0f);
    ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch, 1.0f);
    

    // Função auxiliar para adicionar linha
    auto AddTableRow = [&](const char* label, const char* id, auto&& renderControl) {
        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::Text(label);
        ImGui::TableSetColumnIndex(1);
        ImGui::PushItemWidth(-1);
        renderControl(id);
        ImGui::PopItemWidth();
    };

    #define _AddTableRow(label, func) AddTableRow(label, ## label, func)

    // Exemplo de uso
    AddTableRow("Type", "##type", [&](const char* id) {
        //ImGui::DrawEnumCombo<LightComponent::Type>(id, &light.type);
        float fullWidth = ImGui::GetContentRegionAvail().x;
        float comboWidth = fullWidth * 0.75f; // Por exemplo, 65% para o combo
        float checkboxWidth = fullWidth - comboWidth;

        ImGui::PushItemWidth(comboWidth);
        ImGui::DrawEnumCombo<LightComponent::Type>(id, &light.type);
        ImGui::PopItemWidth();

        ImGui::SameLine();

        ImGui::PushItemWidth(checkboxWidth);
        bool v;
        ImGui::Checkbox("##enabled", &v); // Use um nome válido
        ImGui::PopItemWidth();
    });

    //_AddTableRow("Color", [&](const char* id) {
    AddTableRow("Color", "##color", [&](const char* id) {
        ImGui::ColorEdit4(id, &light.color);
    });

    AddTableRow("Intensity", "##intensity", [&](const char* id) {
        ImGui::DragFloat(id, &light.intensity, 0.025f, 0, 1000, "%.3f", ImGuiSliderFlags_AlwaysClamp);
    });

    AddTableRow("Specular", "##specular", [&](const char* id) {
        ImGui::DragFloat(id, &light.specular, 0.025f, 0, 1, "%.3f", ImGuiSliderFlags_AlwaysClamp);
    });

    AddTableRow("Falloff", "##falloff", [&](const char* id) {
        ImGui::DragFloat(id, &light.falloff, 0.025f, 0, 1, "%.3f", ImGuiSliderFlags_AlwaysClamp);
    });

    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    ImGui::Spacing(); ImGui::Spacing();

    if (light.type == LightComponent::Type::Point) {
        AddTableRow("Radius", "##radius", [&](const char* id) {
            ImGui::DragFloat(id, &light.radius, 0.1f, 0, 1000, "%.3f", ImGuiSliderFlags_AlwaysClamp);
        });
    }

    if (light.type == LightComponent::Type::Spot) {
        AddTableRow("Radius", "##radius_spot", [&](const char* id) {
            ImGui::DragFloat(id, &light.radius, 0.1f, 0, 1000, "%.3f", ImGuiSliderFlags_AlwaysClamp);
        });

        if (light.coneAngleInner > light.coneAngleOuter) {
            light.coneAngleInner = light.coneAngleOuter;
        }

        AddTableRow("Cone Angle Inner", "##coneAngleInner", [&](const char* id) {
            ImGui::DragFloat(id, &light.coneAngleInner, 0.1f, 0, 1000, "%.3f", ImGuiSliderFlags_AlwaysClamp);
        });

        AddTableRow("Cone Angle Outer", "##coneAngleOuter", [&](const char* id) {
            ImGui::DragFloat(id, &light.coneAngleOuter, 0.1f, 0, 1000, "%.3f", ImGuiSliderFlags_AlwaysClamp);
        });
    }

    AddTableRow("Render Shadow", "##renderShadow", [&](const char* id) {
        ImGui::Checkbox(id, &light.renderShadow);
    });

    ImGui::EndTable();
}

void LightComponent::CreateLuaBind(sol::state& lua){
    lua.new_enum(
        "LightComponentType",
        "Directional", LightComponent::Type::Directional,
        "Point", LightComponent::Type::Point,
        "Spot", LightComponent::Type::Spot
    );

    Scene::RegisterMetaComponent<LightComponent>();
    lua.new_usertype<LightComponent>(
        "LightComponent",
        "TypeId", &entt::type_hash<LightComponent>::value,
        sol::call_constructor,
        sol::factories(
            [](){ return LightComponent(); }
        ),
        "type", &LightComponent::type,
        "color", &LightComponent::color,
        "intensity", &LightComponent::intensity,
        "specular", &LightComponent::specular,
        "falloff", &LightComponent::falloff,
        "radius", &LightComponent::radius,
        "coneAngleInner", &LightComponent::coneAngleInner,
        "coneAngleOuter", &LightComponent::coneAngleOuter,
        "renderShadow", &LightComponent::renderShadow,
        "shadowStrength", &LightComponent::shadowStrength,
        "shadowBias", &LightComponent::shadowBias,
        "shadowNormalBias", &LightComponent::shadowNormalBias
    );
}

}