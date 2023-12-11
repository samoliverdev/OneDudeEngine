#pragma once

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include "OD/Renderer/Model.h"
#include "OD/AnimationSystem/Skeleton.h"
#include "OD/AnimationSystem/Clip.h"

namespace OD{

bool AssimpLoadModel(Model& model, std::string const &path, Ref<Shader> customShader, std::vector<Clip>* outClips = nullptr);

}