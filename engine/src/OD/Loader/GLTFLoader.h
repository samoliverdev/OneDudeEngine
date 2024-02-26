#pragma once
#include "OD/Defines.h"
#include "OD/Animation/Pose.h"
#include "OD/Animation/Clip.h"
#include "OD/Animation/Skeleton.h"
#include "OD/Graphics/Mesh.h"
//#include <cgltf.h>

struct cgltf_data;

namespace OD{

OD_API cgltf_data* LoadGLTFFile(const char* path);
void OD_API FreeGLTFFile(cgltf_data* handle);

Pose OD_API LoadRestPose(cgltf_data* data);
std::vector<std::string> OD_API LoadJointNames(cgltf_data* data);
std::vector<Clip> OD_API LoadAnimationClips(cgltf_data* data);

Pose OD_API LoadBindPose(cgltf_data* data);
Skeleton OD_API LoadSkeleton(cgltf_data* data);

std::vector<Ref<Mesh>> OD_API LoadMeshes(cgltf_data* data);

}