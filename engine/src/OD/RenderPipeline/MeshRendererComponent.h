#pragma once

#include "OD/Scene/Scene.h"
#include "OD/Renderer/Culling.h"
#include "OD/Renderer/Mesh.h"
#include "OD/Serialization/Serialization.h"
#include "OD/Core/ImGui.h"
#include "OD/Scene/Scene.h"

#include <vector>
#include <algorithm>

namespace OD{

class StandRenderPipeline;

/*template <class Archive>
void serialize(Archive& ar, Ref<Model>& model){
    LogInfo("Ref<Model> Test Serialize");
}*/

struct MeshRendererComponent{
    friend class StandRenderPipeline;
    friend class StandRenderPipeline2;
    
    static void OnGui(Entity& e);

    inline Ref<Model> GetModel(){ return model; }
    void SetModel(Ref<Model> m);

    inline void SetDefaultAABB(){
        boundingVolume = AABB(Vector3Zero, 1, 1, 1);
    }

    inline int GetSubMeshIndex(){ return subMeshIndex; }
    inline void SetSubMeshIndex(int v){
        subMeshIndex = v;
        if(v < -1) subMeshIndex = -1;
        if(model != nullptr && v >= static_cast<int>(model->meshs.size())) subMeshIndex = model->meshs.size()-1;
    }

    inline std::vector<Ref<Material>>& GetMaterialsOverride(){ return materialsOverride; }

    template<class Archive>
    void save(Archive & ar) const{
        std::string path = model == nullptr ? "" : model->Path();
        std::vector<std::string> materialsOverridePaths;

        for(auto i: materialsOverride){
            materialsOverridePaths.push_back(i == nullptr ? "" : i->Path());
        }

        ar(
            CEREAL_NVP(subMeshIndex),
            CEREAL_NVP(path),
            CEREAL_NVP(materialsOverridePaths)
        );
    }

    template<class Archive>
    void load(Archive & ar){
        std::string path;
        std::vector<std::string> materialsOverridePaths;

        ar(
            CEREAL_NVP(subMeshIndex),
            CEREAL_NVP(path),
            CEREAL_NVP(materialsOverridePaths)
        );


        if(path.empty() == false){
            SetModel(AssetManager::Get().LoadModel(path));
        }

        if(model != nullptr) Assert(materialsOverride.size() == materialsOverride.size()); 

        //if(_materialsOverride.size() != materialsOverride.size()) _materialsOverride.resize(materialsOverride.size());

        int index = 0;
        for(auto i: materialsOverridePaths){
            if(i.empty() == false) materialsOverride[index] = AssetManager::Get().LoadMaterial(i);
            index += 1;
        }
    }

    AABB GetAABB();
    AABB GetGlobalAABB(TransformComponent& transform);

protected:
    Ref<Model> model = nullptr;
    int subMeshIndex = -1;
    std::vector<Ref<Material>> materialsOverride;
    AABB boundingVolume;
    Sphere boundingVolumeSphere;
    bool boundingVolumeIsDirty = true;
};

struct SkinnedMeshRendererComponent: public MeshRendererComponent{
    friend class StandRenderPipeline;

    std::vector<Matrix4> posePalette;

    static void OnGui(Entity& e);

    inline void UpdatePosePalette(){
        GetModel()->skeleton.GetRestPose().GetMatrixPalette(posePalette, model->skeleton.GetInvBindPose());
    }
};

};