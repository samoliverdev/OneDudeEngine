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

    static void Serialize(YAML::Emitter& out, Entity& e);
    static void Deserialize(YAML::Node& in, Entity& e);
    static void OnGui(Entity& e);

    inline Ref<Model> model(){ return _model; }
    inline void model(Ref<Model> m){
        _model = m;
        _materialsOverride.resize(_model->materials.size());
    }

    inline int subMeshIndex(){ return _subMeshIndex; }
    inline void subMeshIndex(int v){
        _subMeshIndex = v;
        if(v < -1) _subMeshIndex = -1;
        if(_model != nullptr && v >= static_cast<int>(_model->meshs.size())) _subMeshIndex = _model->meshs.size()-1;
    }

    inline std::vector<Ref<Material>>& materialsOverride(){ return _materialsOverride; }

    /*template <class Archive>
    void serialize(Archive & ar){
        ar(
            CEREAL_NVP(_subMeshIndex),
            CEREAL_NVP(_model == nullptr ? "None" : _model->path());
        );
    }*/

    template<class Archive>
    void save(Archive & ar) const{
        std::string path = _model == nullptr ? "" : _model->path();
        std::vector<std::string> materialsOverride;

        for(auto i: _materialsOverride){
            materialsOverride.push_back(i == nullptr ? "" : i->path());
        }

        ar(
            CEREAL_NVP(_subMeshIndex),
            CEREAL_NVP(path),
            CEREAL_NVP(materialsOverride)
        );
    }

    template<class Archive>
    void load(Archive & ar){
        std::string path;
        std::vector<std::string> materialsOverride;

        ar(
            CEREAL_NVP(_subMeshIndex),
            CEREAL_NVP(path),
            CEREAL_NVP(materialsOverride)
        );


        if(path.empty() == false){
            model(AssetManager::Get().LoadModel(path));
        }

        if(_model != nullptr) Assert(_materialsOverride.size() == materialsOverride.size()); 

        //if(_materialsOverride.size() != materialsOverride.size()) _materialsOverride.resize(materialsOverride.size());

        int index = 0;
        for(auto i: materialsOverride){
            if(i.empty() == false) _materialsOverride[index] = AssetManager::Get().LoadMaterial(i);
            index += 1;
        }
    }

private:
    Ref<Model> _model = nullptr;
    int _subMeshIndex = -1;
    std::vector<Ref<Material>> _materialsOverride;
    AABB _boundingVolume;
    Sphere _boundingVolumeSphere;
    bool _boundingVolumeIsDirty = true;

    AABB getGlobalAABB(TransformComponent& transform);
};

};