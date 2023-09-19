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

private:
    Ref<Model> _model = nullptr;
    int _subMeshIndex = -1;
    std::vector<Ref<Material>> _materialsOverride;
};

};