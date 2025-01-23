#pragma once
#include "OD/Defines.h"
#include <map>
#include "Clip.h"

namespace OD{

class Mesh;
class Skeleton;

//typedef std::map<int, int> BoneMap;
using BoneMap = std::map<int, int>;

BoneMap OD_API RearrangeSkeleton(Skeleton& skeleton);
void OD_API RearrangeMesh(Mesh& mesh, BoneMap& boneMap);
void OD_API RearrangeClip(Clip& clip, BoneMap& boneMap);
void OD_API RearrangeFastclip(FastClip& clip, BoneMap& boneMap);

}