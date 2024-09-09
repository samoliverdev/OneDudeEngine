#pragma once
#include "OD/Defines.h"
#include "OD/Core/Color.h"
#include "OD/Core/Math.h"
#include "OD/Scene/Scene.h"
#include "OD/Serialization/Serialization.h"
#include "OD/Core/ImGui.h"
#include "OD/Core/Lua.h"

namespace OD{

struct OD_API LightComponent{
    friend class StandRenderPipeline;

    enum class Type{ Directional, Point, Spot };

    Type type = Type::Directional;
    Color color = {1,1,1,1};
    
    float intensity = 1;
    float specular = 1;
    float falloff = 1;

    float radius = 5;

    float coneAngleInner = 45;
    float coneAngleOuter = 45;
    
    bool renderShadow = true;
    float shadowStrength = 1;
    float shadowBias;
    float shadowNormalBias;

    static void OnGui(Entity& e);

    template <class Archive>
    void serialize(Archive & ar){
        ArchiveDump(ar, CEREAL_NVP(type));
        ArchiveDump(ar, CEREAL_NVP(color));
        ArchiveDump(ar, CEREAL_NVP(intensity));
        ArchiveDump(ar, CEREAL_NVP(falloff));
        ArchiveDump(ar, CEREAL_NVP(radius));
        ArchiveDump(ar, CEREAL_NVP(coneAngleInner));
        ArchiveDump(ar, CEREAL_NVP(coneAngleOuter));
        ArchiveDump(ar, CEREAL_NVP(renderShadow));
        ArchiveDump(ar, CEREAL_NVP(shadowStrength));
        ArchiveDump(ar, CEREAL_NVP(shadowBias));
        ArchiveDump(ar, CEREAL_NVP(shadowNormalBias));
    }

    static void CreateLuaBind(sol::state& lua);
};

}