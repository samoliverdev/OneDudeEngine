#pragma once

#include <vector>
#include "CrossFadeTarget.h"
#include "Skeleton.h"

namespace OD{

class CrossFadeController{
public:
    CrossFadeController();
    CrossFadeController(Skeleton& skeleton);
    void SetSkeleton(Skeleton& skeleton);
    void Play(Clip* target);
    void FadeTo(Clip* target, float fadeTime);
    void Update(float dt);
    Pose& GetCurrentPose();
    Clip* GetCurrentClip();
protected:
    std::vector<CrossFadeTarget> _targets;
    Clip* _clip;
    float _time;
    Pose _pose;
    Skeleton _skeleton;
    bool _wasSkeletonSet;   
};

}