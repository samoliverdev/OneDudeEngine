#pragma once

#include <map>
#include "Skeleton.h"
#include "Clip.h"
#include "OD/Renderer/Mesh.h"

namespace OD{

//typedef std::map<int, int> BoneMap;
using BoneMap = std::map<int, int>;

BoneMap RearrangeSkeleton(Skeleton& skeleton);
void RearrangeMesh(Mesh& mesh, BoneMap& boneMap);
void RearrangeClip(Clip& clip, BoneMap& boneMap);
void RearrangeFastclip(FastClip& clip, BoneMap& boneMap);

}