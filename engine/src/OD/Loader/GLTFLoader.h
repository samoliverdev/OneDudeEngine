#pragma once

#include <cgltf.h>
#include "OD/AnimationSystem/Pose.h"
#include "OD/AnimationSystem/Clip.h"
#include <vector>
#include <string>

namespace OD{

cgltf_data* LoadGLTFFile(const char* path);
void FreeGLTFFile(cgltf_data* handle);

Pose LoadRestPose(cgltf_data* data);
std::vector<std::string> LoadJointNames(cgltf_data* data);
std::vector<Clip> LoadAnimationClips(cgltf_data* data);

}