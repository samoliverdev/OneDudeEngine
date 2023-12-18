#pragma once

#include "OD/Core/Math.h"
#include "OD/Scene/Scene.h"
#include "OD/Serialization/Serialization.h"
#include "OD/Core/ImGui.h"

namespace OD{

struct LightComponent{
    friend class StandRenderPipeline;

    enum class Type{ Directional, Point, Spot };

    Type type = Type::Directional;
    Vector3 color = {1,1,1};
    
    float intensity = 1;
    float specular = 1;
    float falloff = 1;

    float radius = 5;

    float coneAngleInner = 45;
    float coneAngleOuter = 45;
    
    bool renderShadow = true;

    static void OnGui(Entity& e);

    template <class Archive>
    void serialize(Archive & ar){
        ar(
            //CEREAL_NVP(type),
            CEREAL_NVP(color),
            CEREAL_NVP(intensity),
            CEREAL_NVP(falloff),
            CEREAL_NVP(radius),
            CEREAL_NVP(coneAngleInner),
            CEREAL_NVP(coneAngleOuter),
            CEREAL_NVP(renderShadow)
        );
    }
};

}