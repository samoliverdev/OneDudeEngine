#include "CameraComponent.h"

namespace OD{

void CameraComponent::Serialize(YAML::Emitter& out, Entity& e){
    out << YAML::Key << "CameraComponent";
    out << YAML::BeginMap;

    auto& cam = e.GetComponent<CameraComponent>();
    out << YAML::Key << "type" << YAML::Value << (int)cam.type;
    out << YAML::Key << "orthographicSize" << YAML::Value << cam.orthographicSize;
    out << YAML::Key << "fieldOfView" << YAML::Value << cam.fieldOfView;
    out << YAML::Key << "nearClipPlane" << YAML::Value << cam.nearClipPlane;
    out << YAML::Key << "farClipPlane" << YAML::Value << cam.farClipPlane;
    
    out << YAML::EndMap;
}

void CameraComponent::Deserialize(YAML::Node& in, Entity& e){
    auto& c = e.AddOrGetComponent<CameraComponent>();
    c.type = (CameraComponent::Type)in["type"].as<int>();
    c.orthographicSize = in["orthographicSize"].as<float>();
    c.fieldOfView = in["fieldOfView"].as<float>();
    c.nearClipPlane = in["nearClipPlane"].as<float>();
    c.farClipPlane = in["farClipPlane"].as<float>();
}

void CameraComponent::OnGui(Entity& e){
    CameraComponent& cam = e.GetComponent<CameraComponent>();

    const char* projectionTypeString[] = {"Perspective", "Orthographic"};
    const char* curProjectionTypeString = projectionTypeString[(int)cam.type];
    if(ImGui::BeginCombo("Projection", curProjectionTypeString)){
        for(int i = 0; i < 2; i++){
            bool isSelected = curProjectionTypeString == projectionTypeString[i];
            if(ImGui::Selectable(projectionTypeString[i], isSelected)){
                curProjectionTypeString = projectionTypeString[i];
                cam.type = (CameraComponent::Type)i;
            }

            if(isSelected) ImGui::SetItemDefaultFocus();
            
        }

        ImGui::EndCombo();
    }

    if(cam.type == CameraComponent::Type::Orthographic){
        ImGui::DragFloat("size", &cam.orthographicSize);
    }

    if(cam.type == CameraComponent::Type::Perspective){
        ImGui::DragFloat("fieldOfView", &cam.fieldOfView);
    }

    ImGui::DragFloat("nearClipPlane", &cam.nearClipPlane);
    ImGui::DragFloat("farClipPlane", &cam.farClipPlane);
}

}