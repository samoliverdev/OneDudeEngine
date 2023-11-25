#pragma once

#include "OD/Defines.h"
#include "OD/Core/Asset.h"
#include "OD/Core/Math.h"
#include "Mesh.h"
#include "Shader.h"
#include "Material.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include "OD/AnimationSystem/Skeleton.h"

namespace OD{

class Model: public Asset{
public:
    std::vector<Ref<Mesh>> meshs;
    std::vector<Ref<Material>> materials;
    std::vector<Matrix4> matrixs;
    Skeleton skeleton;
    
    static Ref<Model> CreateFromFile(std::string const &path, Ref<Shader> customShader = nullptr);
};

}