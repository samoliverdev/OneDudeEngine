#include "EnvironmentComponent.h"

namespace OD{

void EnvironmentComponent::Serialize(YAML::Emitter& out, Entity& e){

}

void EnvironmentComponent::Deserialize(YAML::Node& in, Entity& e){

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