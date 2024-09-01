#pragma once

#include "Pose.h"
#include "Clip.h"
#include "Skeleton.h"

namespace OD{

bool OD_API IsInHierarchy(Pose& pose, unsigned int parent, unsigned int search);
void OD_API Blend(Pose& output, Pose& a, Pose& b, float t, int blendroot);

Pose OD_API MakeAdditivePose(Skeleton& skeleton, Clip& clip);
void OD_API Add(Pose& output, Pose& inPose, Pose& addPose, Pose& additiveBasePose, int blendroot);

}