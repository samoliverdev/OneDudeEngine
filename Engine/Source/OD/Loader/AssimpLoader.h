#ifdef USE_ASSIMP
#pragma once
#include "OD/Defines.h"
#include "OD/Graphics/Model.h"
#include "OD/Animation/Skeleton.h"
#include "OD/Animation/Clip.h"

namespace OD{

class SubShader;

bool OD_API AssimpLoadModel(
    Model& model, 
    std::string const &path, 
    ModelLoadSettings loadSettings, 
    std::vector<Clip>* outClips = nullptr
);

}
#endif