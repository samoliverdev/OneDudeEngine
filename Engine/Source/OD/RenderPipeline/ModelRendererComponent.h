#pragma once
#include "OD/Defines.h"
#include "OD/Graphics/Culling.h"
#include "OD/Graphics/Model.h"
#include "OD/Graphics/UniformBuffer.h"
#include "OD/Serialization/Serialization.h"
#include "OD/Scene/Ecs.h"
#include "OD/Core/Color.h"

namespace OD{

class Model;
class Scene;

class StandRenderPipeline;

struct OD_API StaticRendererComponent{
    struct OD_API alignas(16) StaticData{
        Matrix4 m;
        AABB aabb;
        bool isDirt = true;
    };
    AlignedVector<StaticData> staticDatas;

    template <class Archive> void serialize(Archive& ar){}
};

struct OD_API ModelRendererComponent{
    friend class StandRenderPipeline;
    friend class RenderContext;
    friend struct SkinnedModelRendererComponent;

    struct alignas(16) RenderData{
        Matrix4 model;
        AABB aabb;
    };

    Pose finalPose;
    Transform localTransform;
    AlignedVector<RenderData> renderData;
    Ref<Material> customShadowPass = nullptr;
    Vector4 customData;
    bool useCustomData = false;
    bool castShadow = true;
    bool draw = true;

    static void OnGui(Entity& e, Scene& scene);

    inline Ref<Model> GetModel() const { return model; }
    void SetModel(Ref<Model> m);

    inline void SetAABB(Vector3 center = Vector3Zero, Vector3 size = Vector3One){
        boundingVolume = AABB(center, size.x, size.y, size.z);
    }

    inline int GetSubMeshIndex(){ return subMeshIndex; }
    inline void SetSubMeshIndex(int v){
        subMeshIndex = v;
        if(v < -1) subMeshIndex = -1;
        if(model != nullptr && v >= static_cast<int>(model->meshs.size())) subMeshIndex = model->meshs.size()-1;
    }

    inline std::vector<Ref<Material>>& GetMaterialsOverride(){ return materialsOverride; }
    inline const std::vector<bool>& GetRenderTargetVisibility() const { return renderTargetVisibility; }

    AABB GetAABB() const;
    AABB GetGlobalAABB(TransformComponent& transform);
    AABB GetGlobalAABB(Transform& transform);

    template <class Archive>
    void serialize(Archive& ar){
        ArchiveDumpNVP(ar, localTransform);
        ArchiveDumpNVP(ar, subMeshIndex);
        ArchiveDumpNVP(ar, boundingVolume);
        ArchiveDumpNVP(ar, renderTargetVisibility);

        ArchiveDumpNVP(ar, castShadow);

        AssetRefSerialize<Model> modelRef(model);
        ArchiveDumpNVP(ar, modelRef);

        AssetVectorRefSerialize<Material> materialVectorRef(materialsOverride);
        ArchiveDumpNVP(ar, materialVectorRef);

        AssetRefSerialize<Material> _customShadowPass(customShadowPass);
        ArchiveDumpNamed(ar, "customShadowPass", _customShadowPass);

        if(GetRenderTargetVisibility().size() != model->renderTargets.size()){
            renderTargetVisibility.resize(model->renderTargets.size());
            for(int i = 0; i < renderTargetVisibility.size(); i++){
                renderTargetVisibility[i] = true;
            }
        }
    }
protected:
    AABB boundingVolume;
    std::vector<bool> renderTargetVisibility;
    std::vector<Ref<Material>> materialsOverride;
    Ref<Model> model = nullptr;
    int subMeshIndex = -1;
    //Sphere boundingVolumeSphere;
    bool boundingVolumeIsDirty = true;
};

struct OD_API SkinnedModelRendererComponent: public ModelRendererComponent{
    friend class StandRenderPipeline;
    friend class RenderContext;
    
    Pose finalPose;
    Transform skeletonTransform;
    AlignedVector<Matrix4> posePalette;
    std::vector<Entity> skeletonEntities;
    std::vector<Entity> skeletonEntities2;
    std::vector<int> skeletonSockets;
    Ref<UniformBuffer> skinnedData = nullptr;
    bool useSkinnedData = false;
    bool postUpdatePosePalette = false;
    bool updateWhenOffscreen = false;

    static void OnGui(Entity& e, Scene& scene);
    
    //INFO: Bug if is called in editor scene the skeletonEntities are linked with editor scene not the running scene
    void CreateSkeletonEntites(Entity& selfEntity, Scene& scene);
    void UpdateSkeletonEntites(Pose& animatedPose, Scene& scene);
    void UpdateSkeletonEntitesIn(Pose& animatedPose, Scene& scene);

    void CreateSkeletonEntites2(Entity& selfEntity, Scene& scene);
    void UpdateSkeletonEntites2(Pose& animatedPose, Scene& scene);

    inline void UpdatePosePalette(){
        finalPose = model->skeleton.GetRestPose();
        GetModel()->skeleton.GetRestPose().GetMatrixPalette(posePalette, model->skeleton.GetInvBindPose());
    }

    template <class Archive>
    void serialize(Archive& ar){
        ArchiveDumpNVP(ar, localTransform);
        ArchiveDumpNVP(ar, subMeshIndex);
        ArchiveDumpNVP(ar, boundingVolume);
        ArchiveDumpNVP(ar, renderTargetVisibility);

        ArchiveDumpNVP(ar, castShadow);

        ArchiveDumpNVP(ar, updateWhenOffscreen);
        ArchiveDumpNVP(ar, useSkinnedData);

        AssetRefSerialize<Model> modelRef(model);
        ArchiveDumpNVP(ar, modelRef);

        AssetVectorRefSerialize<Material> materialVectorRef(materialsOverride);
        ArchiveDumpNVP(ar, materialVectorRef);

        AssetRefSerialize<Material> _customShadowPass(customShadowPass);
        ArchiveDumpNamed(ar, "customShadowPass", _customShadowPass);
    }
};

struct OD_API GizmosDrawComponent{
    Vector3 center = {0, 0, 0};
    Vector3 size = {0.05f, 0.05f, 0.05f};
    Color color = {0, 1, 0, 1};

    template <class Archive> 
    void serialize(Archive& ar){
        ArchiveDumpNVP(ar, center);
        ArchiveDumpNVP(ar, size);   
        ArchiveDumpNVP(ar, color);    
    }
};

};