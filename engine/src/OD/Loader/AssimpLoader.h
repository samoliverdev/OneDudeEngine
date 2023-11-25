#pragma once

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include "OD/Renderer/Model.h"
#include "OD/AnimationSystem/Skeleton.h"

namespace OD{

Ref<Model> AssimpLoadModel(std::string const &path, Ref<Shader> customShader);

}