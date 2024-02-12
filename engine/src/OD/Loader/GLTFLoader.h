#pragma once

#include <cgltf.h>
#include "OD/Animation/Pose.h"
#include "OD/Animation/Clip.h"
#include "OD/Animation/Skeleton.h"
#include "OD/Graphics/Mesh.h"
#include <vector>
#include <string>

namespace OD{

cgltf_data* LoadGLTFFile(const char* path);
void FreeGLTFFile(cgltf_data* handle);

Pose LoadRestPose(cgltf_data* data);
std::vector<std::string> LoadJointNames(cgltf_data* data);
std::vector<Clip> LoadAnimationClips(cgltf_data* data);

Pose LoadBindPose(cgltf_data* data);
Skeleton LoadSkeleton(cgltf_data* data);

std::vector<Ref<Mesh>> LoadMeshes(cgltf_data* data);

}