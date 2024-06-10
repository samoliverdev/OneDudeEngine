#pragma once
#include "OD/Defines.h"
#include "OD/Scene/Scene.h"
#include "OD/Graphics/Culling.h"
#include "OD/Graphics/Mesh.h"
#include "OD/Serialization/Serialization.h"
#include "OD/Core/ImGui.h"
#include "OD/Scene/Scene.h"

namespace OD{

class StandRenderPipeline;

/*template <class Archive>
void serialize(Archive& ar, Ref<Model>& model){
    LogInfo("Ref<Model> Test Serialize");
}*/

//TODO: Make Serializable
struct OD_API ModelRendererComponent{
    friend class StandRenderPipeline;

    Transform localTransform;
    
    static void OnGui(Entity& e);

    inline Ref<Model> GetModel(){ return model; }
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

    template <class Archive>
    void serialize(Archive& ar){
        ArchiveDumpNVP(ar, localTransform);
        ArchiveDumpNVP(ar, subMeshIndex);

        AssetRef<Model> modelRef(model);
        ArchiveDumpNVP(ar, modelRef);
        AssetVectorRef<Material> materialVectorRef(materialsOverride);
        ArchiveDumpNVP(ar, materialVectorRef);
    }

    /*template<class Archive>
    void save(Archive & ar) const{
        std::string path = model == nullptr ? "" : model->Path();
        std::vector<std::string> materialsOverridePaths;

        for(auto i: materialsOverride){
            materialsOverridePaths.push_back(i == nullptr ? "" : i->Path());
        }

        ArchiveDump(ar, CEREAL_NVP(subMeshIndex));
        ArchiveDump(ar, CEREAL_NVP(path));
        ArchiveDump(ar, CEREAL_NVP(materialsOverridePaths));
    }

    template<class Archive>
    void load(Archive & ar){
        std::string path;
        std::vector<std::string> materialsOverridePaths;

        ArchiveDump(ar, CEREAL_NVP(subMeshIndex));
        ArchiveDump(ar, CEREAL_NVP(path));
        ArchiveDump(ar, CEREAL_NVP(materialsOverridePaths));


        if(path.empty() == false){
            SetModel(AssetManager::Get().LoadAsset<Model>(path));
        }

        if(model != nullptr) Assert(materialsOverride.size() == materialsOverride.size()); 

        int index = 0;
        for(auto i: materialsOverridePaths){
            if(i.empty() == false) materialsOverride[index] = AssetManager::Get().LoadAsset<Material>(i);
            index += 1;
        }
    }*/

    AABB GetAABB();
    AABB GetGlobalAABB(TransformComponent& transform);
    AABB GetGlobalAABB(Transform& transform);

protected:
    Ref<Model> model = nullptr;
    int subMeshIndex = -1;
    std::vector<Ref<Material>> materialsOverride;
    AABB boundingVolume;
    Sphere boundingVolumeSphere;
    bool boundingVolumeIsDirty = true;
};

struct OD_API SkinnedModelRendererComponent: public ModelRendererComponent{
    friend class StandRenderPipeline;

    std::vector<Matrix4> posePalette;

    static void OnGui(Entity& e);

    inline void UpdatePosePalette(){
        GetModel()->skeleton.GetRestPose().GetMatrixPalette(posePalette, model->skeleton.GetInvBindPose());
    }
};

};