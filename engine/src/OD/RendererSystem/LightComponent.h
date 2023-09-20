#pragma once

#include "OD/Core/Math.h"
#include "OD/Scene/Scene.h"
#include "StandRendererSystem.h"
#include "OD/Serialization/Serialization.h"
#include "OD/Core/ImGui.h"

namespace OD{

struct LightComponent{
    friend class StandRendererSystem;

    static void Serialize(YAML::Emitter& out, Entity& e);
    static void Deserialize(YAML::Node& in, Entity& e);
    static void OnGui(Entity& e);

    enum class Type{ Directional, Point, Spot };

    Type type = Type::Directional;
    Vector3 color = {1,1,1};
    float intensity = 1;
    float radius = 5;
    bool renderShadow = true;
};

}