#pragma once

#include "OD/Defines.h"
#include "OD/Serialization/Serialization.h"
#include "OD/Core/Asset.h"
#include "OD/Core/Math.h"
#include "Mesh.h"
#include "Shader.h"
#include "Material.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include "OD/AnimationSystem/Skeleton.h"
#include "OD/AnimationSystem/Clip.h"

namespace OD{

class Model: public Asset{
public:
    struct RenderTarget{
        int meshIndex;
        int materialIndex;
        int bindPoseIndex;
    };

    std::vector<RenderTarget> renderTargets;

    std::vector<Ref<Mesh>> meshs;
    std::vector<Ref<Material>> materials;
    std::vector<Ref<Texture2D>> textures;
    std::vector<Matrix4> matrixs;
    
    Skeleton skeleton;

    std::vector<Ref<Clip>> animationClips;
    
    static Ref<Model> CreateFromFile(std::string const &path, Ref<Shader> customShader = nullptr);
    
    /*
    friend class cereal::access;
    template <class Archive>
    void serialize(Archive & ar){
        ar(CEREAL_NVP(_path));
    }

    // We could define load_and_construct internally:
    template <class Archive>
    static void load_and_construct(Archive & ar, cereal::construct<Model> & construct){
      int x;
      ar(x);
      construct(x);
    }*/
};

}