#include "EnvironmentComponent.h"

namespace OD{

void EnvironmentComponent::Serialize(YAML::Emitter& out, Entity& e){
    out << YAML::Key << "EnvironmentComponent";
    out << YAML::BeginMap;

    auto& c = e.GetComponent<EnvironmentComponent>();
    out << YAML::Key << "settings.ambient" << YAML::Value << c.settings.ambient;
    
    out << YAML::EndMap;
}

void EnvironmentComponent::Deserialize(YAML::Node& in, Entity& e){
    auto& c = e.AddOrGetComponent<EnvironmentComponent>();
    c.settings.ambient = in["settings.ambient"].as<Vector3>();
}

void EnvironmentComponent::OnGui(Entity& e){
    EnvironmentComponent& environment = e.GetComponent<EnvironmentComponent>();

    float ambient[] = {
        environment.settings.ambient.x, 
        environment.settings.ambient.y, 
        environment.settings.ambient.z
    };
    if(ImGui::ColorEdit3("ambient", ambient)){
        environment.settings.ambient = Vector3(ambient[0], ambient[1], ambient[2]);
    }
}

}