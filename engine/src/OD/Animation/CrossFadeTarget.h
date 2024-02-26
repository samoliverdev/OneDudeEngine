#pragma once

#include "Clip.h"
#include "Pose.h"

namespace OD{

struct OD_API CrossFadeTarget{
    Pose pose;
    Clip* clip;
    float time;
    float duration;
    float elapsed;

    inline CrossFadeTarget():
        clip(0), time(0.0f), duration(0.0f), elapsed(0.0f){}
    
    inline CrossFadeTarget(Clip* inTarget, Pose& inPose, float inDuration):
        clip(inTarget), time(inTarget->GetStartTime()), 
        pose(inPose), duration(inDuration), elapsed(0.0f){}
};

}