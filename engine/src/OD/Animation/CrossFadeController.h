#pragma once

#include <vector>
#include "CrossFadeTarget.h"
#include "Skeleton.h"

namespace OD{

class OD_API CrossFadeController{
public:
    CrossFadeController();
    CrossFadeController(Skeleton& skeleton);
    void SetSkeleton(Skeleton& skeleton);
    void Play(Clip* target);
    void FadeTo(Clip* target, float fadeTime);
    void Update(float dt);
    Pose& GetCurrentPose();
    Clip* GetCurrentClip();
    
    inline bool WasSkeletonSet(){ return wasSkeletonSet; }

protected:
    std::vector<CrossFadeTarget> targets;
    Clip* clip;
    float time;
    Pose pose;
    Skeleton skeleton;
    bool wasSkeletonSet;   
};

}