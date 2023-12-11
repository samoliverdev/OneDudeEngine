#include "CrossFadeController.h"
#include "Blending.h"

namespace OD{

CrossFadeController::CrossFadeController(){
    clip = 0;
    time = 0.0f;
    wasSkeletonSet = false;
}

CrossFadeController::CrossFadeController(Skeleton& inSkeleton){
    clip = 0;
    time = 0.0f;
    SetSkeleton(inSkeleton);
}

void CrossFadeController::SetSkeleton(Skeleton& inSkeleton){
    skeleton = inSkeleton;
    pose = skeleton.GetRestPose();
    wasSkeletonSet = true;
}

void CrossFadeController::Play(Clip* target){
    targets.clear();
    clip = target;
    pose = skeleton.GetRestPose();
    time = target->GetStartTime();
}

void CrossFadeController::FadeTo(Clip* target, float fadeTime){
    if(clip == 0){
        Play(target);
        return;
    }

    if(targets.size() >= 1){
        //Clip* clip = _targets[_targets.size()-1].clip;
        //if(clip == target) return;
        if(targets[targets.size()-1].clip == target) return;
    } else {
        if(clip == target) return;
    }

    targets.push_back(CrossFadeTarget(target, skeleton.GetRestPose(), fadeTime));
}

void CrossFadeController::Update(float dt){
    if(clip == 0 || !wasSkeletonSet) return;

    unsigned int numTargets = targets.size();
    for(unsigned int i = 0; i < numTargets; i++){
        if(targets[i].elapsed >= targets[i].duration){
            clip = targets[i].clip;
            time = targets[i].time;
            pose = targets[i].pose;
            targets.erase(targets.begin() + i);
            break;
        }
    }

    numTargets = targets.size();
    pose = skeleton.GetRestPose();
    time = clip->Sample(pose, time + dt);

    for(unsigned int i = 0; i < numTargets; i++){
        CrossFadeTarget& target = targets[i];
        target.time = target.clip->Sample(target.pose, target.time + dt);
        target.elapsed += dt;
        float t = target.elapsed / target.duration;
        if(t > 1.0f){ t = 1.0f; }
        Blend(pose, pose, target.pose, t, -1);
    }
}

Pose& CrossFadeController::GetCurrentPose(){
    return pose;
}

Clip* CrossFadeController::GetCurrentClip(){
    return clip;
}

}