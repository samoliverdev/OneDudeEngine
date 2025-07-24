#pragma once
#include "OD/Defines.h"
#include "CrossFadeTarget.h"
#include "Skeleton.h"

namespace OD{

class OD_API CrossFadeController{
public:
    bool stopNoLoopAnim = true;

    CrossFadeController();
    CrossFadeController(Skeleton& skeleton);
    void SetSkeleton(Skeleton& skeleton);
    void Play(Clip* target);
    void FadeTo(Clip* target, float fadeTime);
    void FadeTo2(Clip* target, float fadeTime);
    void Update(float dt);
    void Update(float dt, Pose& pose); //INFO: Experimental
    Pose& GetCurrentPose();
    Clip* GetCurrentClip();

    float GetCurrentNormalizedTime();
    
    inline bool WasSkeletonSet(){ return wasSkeletonSet; }
    inline Skeleton& GetSkeleton(){ return skeleton; }
    inline float GetCurrentTime(){ return time; }
    
protected:
    std::vector<CrossFadeTarget> targets;
    Clip* clip;
    float time;
    Pose pose;
    Skeleton skeleton;
    bool wasSkeletonSet = false;   

    float internalTime = 0.0f;
    float lastFadeTime = 0.0f;     // new member
    Clip* lastFadeTarget = nullptr;
    float fadeDebounce = 0.5f;     // seconds
};

}