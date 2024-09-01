#pragma once
#include "OD/Defines.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include "OD/Graphics/Model.h"
#include "OD/Animation/Skeleton.h"
#include "OD/Animation/Clip.h"

namespace OD{

bool OD_API AssimpLoadModel(Model& model, std::string const &path, Ref<Shader> customShader, std::vector<Clip>* outClips = nullptr);

}