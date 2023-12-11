#pragma once

#include "OD/Scene/Scene.h"
#include "OD/Scene/Culling.h"
#include "OD/Renderer/Mesh.h"
#include "OD/Serialization/Serialization.h"
#include "OD/Core/ImGui.h"

#include <vector>
#include <algorithm>

namespace OD{

class StandRendererSystem;

/*template <class Archive>
void serialize(Archive& ar, Ref<Model>& model){
    LogInfo("Ref<Model> Test Serialize");
}*/

struct MeshRendererComponent{
    friend class StandRendererSystem;
    
    static void OnGui(Entity& e);

    inline Ref<Model> GetModel(){ return model; }
    inline void SetModel(Ref<Model> m){
        model = m;
        materialsOverride.resize(model->materials.size());
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

private:
    Ref<Model> model = nullptr;
    int subMeshIndex = -1;
    std::vector<Ref<Material>> materialsOverride;
    AABB boundingVolume;
    Sphere boundingVolumeSphere;
    bool boundingVolumeIsDirty = true;

    AABB getGlobalAABB(TransformComponent& transform);
};

};