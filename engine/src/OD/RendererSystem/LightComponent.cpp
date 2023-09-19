#include "LightComponent.h"

namespace OD{

void LightComponent::Serialize(YAML::Emitter& out, Entity& e){

}

void LightComponent::Deserialize(YAML::Node& in, Entity& e){

}

void LightComponent::OnGui(Entity& e){
    LightComponent& light = e.GetComponent<LightComponent>();

    const char* projectionTypeString[] = {"Directional", "Point", "Spot"};
    const char* curProjectionTypeString = projectionTypeString[(int)light.type];
    if(ImGui::BeginCombo("Type", curProjectionTypeString)){
        for(int i = 0; i < 2; i++){
            bool isSelected = curProjectionTypeString == projectionTypeString[i];
            if(ImGui::Selectable(projectionTypeString[i], isSelected)){
                curProjectionTypeString = projectionTypeString[i];
                light.type = (LightComponent::Type)i;
            }

            if(isSelected) ImGui::SetItemDefaultFocus();
            
        }

        ImGui::EndCombo();
    }

    float color[] = {light.color.x, light.color.y, light.color.z};
    if(ImGui::ColorEdit3("color", color)){
        light.color = Vector3(color[0], color[1], color[2]);
    }

    ImGui::DragFloat("intensity", &light.intensity, 0.025f, 0, 1000, "%.3f", ImGuiSliderFlags_AlwaysClamp);

    if(light.type == LightComponent::Type::Point){
        ImGui::DragFloat("radus", &light.radius, 0.1f, 0, 1000, "%.3f", ImGuiSliderFlags_AlwaysClamp);
    }

    ImGui::Checkbox("renderShadow", &light.renderShadow);
}

}