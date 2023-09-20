#pragma once

#include "StandRendererSystem.h"
#include "OD/Serialization/Serialization.h"
#include "OD/Core/ImGui.h"

namespace OD{

struct EnvironmentSettings{
    Vector3 ambient = {0.1f, 0.1f, 0.1f};
};

struct EnvironmentComponent{
    static void Serialize(YAML::Emitter& out, Entity& e);
    static void Deserialize(YAML::Node& in, Entity& e);
    static void OnGui(Entity& e);

    EnvironmentSettings settings;
};


}