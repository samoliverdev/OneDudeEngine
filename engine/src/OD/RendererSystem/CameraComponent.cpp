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
    
}

}