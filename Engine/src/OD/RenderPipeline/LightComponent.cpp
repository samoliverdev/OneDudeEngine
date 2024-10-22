#include "LightComponent.h"
#include "OD/Serialization/CerealImGui.h"

namespace OD{

void LightComponent::OnGui(Entity& e){
    LightComponent& light = e.GetComponent<LightComponent>();

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

    ImGui::ColorEdit4("color", &light.color);

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

    ImGui::Checkbox("renderShadow", &light.renderShadow);
}

void LightComponent::CreateLuaBind(sol::state& lua){
    lua.new_usertype<LightComponent>(
        "LightComponent",
        "TypeId", &entt::type_hash<LightComponent>::value,
        sol::call_constructor,
        sol::factories(
            [](){ return LightComponent(); }
        )
    );
}

}