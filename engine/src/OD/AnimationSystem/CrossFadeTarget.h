#pragma once

#include "Clip.h"
#include "Pose.h"

namespace OD{

struct CrossFadeTarget{
    Pose pose;
    Clip* clip;
    float time;
    float duration;
    float elapsed;

    inline CrossFadeTarget():
        clip(0), time(0.0f), duration(0.0f), elapsed(0.0f){}
    
    inline CrossFadeTarget(Clip* _target, Pose& _pose, float _duration):
        clip(_target), time(_target->GetStartTime()), 
        pose(_pose), duration(_duration), elapsed(0.0f){}
};

}