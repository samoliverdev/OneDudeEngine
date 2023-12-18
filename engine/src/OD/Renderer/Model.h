#pragma once

#include "OD/Defines.h"
#include "OD/Serialization/Serialization.h"
#include "OD/Core/Asset.h"
#include "OD/Core/Math.h"
#include "Mesh.h"
#include "Shader.h"
#include "Material.h"
#include "Culling.h"
#include "OD/Animation/Skeleton.h"
#include "OD/Animation/Clip.h"

namespace OD{

/*
struct AABB;
struct Sphere;
*/

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
    
    static bool CreateFromFile(Model& model, std::string const &path, Ref<Shader> customShader = nullptr);
    static AABB GenerateAABB(Model& model);
    static Sphere GenerateSphereBV(Model& model);
};

}