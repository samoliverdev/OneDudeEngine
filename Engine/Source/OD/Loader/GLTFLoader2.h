#pragma once
#include "OD/Defines.h"
#include "OD/Graphics/Model.h"
#include "OD/Animation/Skeleton.h"
#include "OD/Animation/Clip.h"

namespace OD{

class Shader;

bool OD_API GltfLoadModel(
    Model& model, 
    std::string const &path, 
    ModelLoadSettings loadSettings, 
    std::vector<Clip>* outClips = nullptr
);

}