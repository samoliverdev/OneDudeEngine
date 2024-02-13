#pragma once
#include "OD/Scene/Scene.h"
#include "OD/Graphics/Mesh.h"
#include "OD/Graphics/Material.h"

namespace OD{

struct MeshRendererComponent{
    Ref<Mesh> mesh;
    Ref<Material> material;

    inline static void OnGui(Entity& e){}

    template<class Archive>
    void serialize(Archive& ar){}
};

}