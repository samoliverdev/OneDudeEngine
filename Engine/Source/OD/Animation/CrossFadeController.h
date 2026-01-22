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
    void Play(ClipT* target);
    void FadeTo(ClipT* target, float fadeTime);
    void FadeTo2(ClipT* target, float fadeTime);
    void Update(float dt);
    void Update(float dt, Pose& pose); //INFO: Experimental
    Pose& GetCurrentPose();
    ClipT* GetCurrentClip();

    float GetCurrentNormalizedTime();

    bool WillOrPlay(ClipT* target);
    
    inline bool WasSkeletonSet(){ return wasSkeletonSet; }
    inline Skeleton& GetSkeleton(){ return skeleton; }
    inline float GetCurrentTime(){ return time; }

    inline const Transform& RootDelta(){ return frameDelta; }

    int rootMotionIndex = -1;
    Vector3 rootMotionPosMask = {1, 1, 0};
    
protected:
    Transform prevRoot;
    Transform frameDelta;
    float prevTime = 0.0f;
    bool hasPrevRoot = false;
    std::vector<CrossFadeTarget> targets;
    ClipT* clip;
    float time;
    Pose pose;
    Skeleton skeleton;
    bool wasSkeletonSet = false;   

    float internalTime = 0.0f;
    float lastFadeTime = 0.0f;     // new member
    ClipT* lastFadeTarget = nullptr;
    float fadeDebounce = 0.5f;     // seconds
};

}