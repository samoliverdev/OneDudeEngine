#pragma once
#include "OD/Defines.h"
#include "OD/Scene/Scene.h"
#include "OD/Graphics/Mesh.h"
#include "OD/Graphics/Material.h"
#include "OD/Serialization/Serialization.h"
#include "OD/Core/Resource.h"

namespace OD{

struct OD_API DecalRendererComponent{
    Vector4 perInstanceData = Vector4Zero;
    Vector3 offset;
    Vector3 size = {1, 1, 1};
    Ref<Material> material = CreateRef<Material>(ResourceManager::Get().LoadByPath<Shader>("Engine/Shaders/DecalTest.glsl"));// nullptr;
    int customLayerIndex = -1;
    bool useCustomOffsetAndSize = false;

    //Ref<Mesh> mesh = nullptr;

    static void OnGui(Entity& e, Scene& scene);

    template<class Archive>
    void serialize(Archive& ar){
        ArchiveDumpNVP(ar, perInstanceData);

        ArchiveDumpNVP(ar, offset);
        ArchiveDumpNVP(ar, size);

        ResourceRefSerialize<Material> materialRef(material);
        ArchiveDumpNVP(ar, materialRef);

        ArchiveDumpNVP(ar, customLayerIndex);

        ArchiveDumpNVP(ar, useCustomOffsetAndSize);
    }
};

}