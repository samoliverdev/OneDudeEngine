#pragma once

#include "OD/Scene/Scene.h"
#include "OD/Renderer/Mesh.h"
#include "StandRendererSystem.h"
#include "OD/Serialization/Serialization.h"
#include "OD/Core/ImGui.h"

#include <vector>
#include <algorithm>

namespace OD{

struct MeshRendererComponent{
    static void Serialize(YAML::Emitter& out, Entity& e);
    static void Deserialize(YAML::Node& in, Entity& e);
    static void OnGui(Entity& e);

    Ref<Model> mesh;
    int subMeshIndex = -1;
    Ref<Material> materialOverride;
};

};