#pragma once

#include "Pose.h"
#include "Clip.h"
#include "Skeleton.h"

namespace OD{

bool IsInHierarchy(Pose& pose, unsigned int parent, unsigned int search);
void Blend(Pose& output, Pose& a, Pose& b, float t, int blendroot);

Pose MakeAdditivePose(Skeleton& skeleton, Clip& clip);
void Add(Pose& output, Pose& inPose, Pose& addPose, Pose& additiveBasePose, int blendroot);

}