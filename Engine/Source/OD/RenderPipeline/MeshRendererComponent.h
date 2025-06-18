#pragma once
#include "OD/Defines.h"
#include "OD/Scene/Scene.h"
#include "OD/Graphics/Mesh.h"
#include "OD/Graphics/Material.h"
#include "OD/Animation/Skeleton.h"

namespace OD{

struct OD_API MeshRendererComponent{
    AABB boundingVolume;
    Ref<Mesh> mesh;
    Ref<Material> material;
    Ref<Material> customShadowPass;

    bool useCustomData = false;
    Vector4 customData;

    struct RenderData{
        Matrix4 model;
        AABB aabb;
    };
    RenderData renderData;

    static void OnGui(Entity& e, Scene& scene);

    template<class Archive>
    void serialize(Archive& ar){
        //AssetRefSerialize<Mesh> meshRef(mesh);
        //ArchiveDumpNVP(ar, meshRef);

        AssetRefSerialize<Material> materialRef(material);
        ArchiveDumpNVP(ar, materialRef);
    }

    void UpdateAABB(Vector3 scale = Vector3One);
    AABB GetGlobalAABB(TransformComponent& transform);
};

struct OD_API SkinnedMeshRendererComponent: public MeshRendererComponent{
    Skeleton skeleton;
    Pose finalPose;
    bool postUpdatePosePalette = false;
    AlignedVector<Matrix4> posePalette;

    template<class Archive>
    void serialize(Archive& ar){
        AssetRefSerialize<Material> materialRef(material);
        ArchiveDumpNVP(ar, materialRef);
    }

    inline void UpdatePosePalette(){
        skeleton.GetRestPose().GetMatrixPalette(posePalette, skeleton.GetInvBindPose());
    }
};

}