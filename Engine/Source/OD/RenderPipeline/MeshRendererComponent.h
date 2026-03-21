#pragma once
#include "OD/Defines.h"
#include "OD/Scene/ECS.h"
#include "OD/Graphics/Culling.h"
#include "OD/Graphics/Mesh.h"
#include "OD/Graphics/Material.h"
#include "OD/Animation/Skeleton.h"

namespace OD{

class Scene;
struct TransformComponent;

struct OD_API MeshRendererComponent{
    struct RenderData{
        Matrix4 model;
        AABB aabb;
    };
    
    AABB boundingVolume;
    RenderData renderData;
    Ref<Mesh> mesh = nullptr;
    Ref<Material> material = nullptr;
    Ref<Material> customShadowPass = nullptr;
    Vector4 customData;
    bool useCustomData = false;

    static void OnGui(Entity& e, Scene& scene);

    template<class Archive>
    void serialize(Archive& ar){
        //AssetRefSerialize<Mesh> meshRef(mesh);
        //ArchiveDumpNVP(ar, meshRef);

        //AssetRefSerialize<Material> materialRef(material);
        //ArchiveDumpNVP(ar, materialRef);
    }

    void UpdateAABB(Vector3 scale = Vector3One);
    AABB GetGlobalAABB(TransformComponent& transform);
};

struct OD_API SkinnedMeshRendererComponent: public MeshRendererComponent{
    Skeleton skeleton;
    Pose finalPose;
    AlignedVector<Matrix4> posePalette;
    bool postUpdatePosePalette = false;

    template<class Archive>
    void serialize(Archive& ar){
        //AssetRefSerialize<Material> materialRef(material);
        //ArchiveDumpNVP(ar, materialRef);
    }

    inline void UpdatePosePalette(){
        skeleton.GetRestPose().GetMatrixPalette(posePalette, skeleton.GetInvBindPose());
    }
};

}