#include "CrossFadeController.h"
#include "Blending.h"
#include "OD/Core/Application.h"

namespace OD{

CrossFadeController::CrossFadeController(){
    clip = nullptr;
    time = 0.0f;
    wasSkeletonSet = false;
}

CrossFadeController::CrossFadeController(Skeleton& inSkeleton){
    clip = nullptr;
    time = 0.0f;
    SetSkeleton(inSkeleton);
}

void CrossFadeController::SetSkeleton(Skeleton& inSkeleton){
    skeleton = inSkeleton;
    pose = skeleton.GetRestPose();
    wasSkeletonSet = true;
}

void CrossFadeController::Play(Clip* target){
    if(target == nullptr){
        targets.clear();
        clip = nullptr;
        time = 0;
        return;
    }

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
        if(targets[targets.size()-1].clip == target) return;
    } else {
        if(clip == target) return;
    }

    targets.push_back(CrossFadeTarget(target, skeleton.GetRestPose(), fadeTime));
}

// Expereimenta, To Avoid Flicking
void CrossFadeController::FadeTo2(Clip* target, float fadeTime) {
    if(!wasSkeletonSet || target == nullptr) return;

    float currentTime = internalTime; // Use your engine’s time
    if(target == lastFadeTarget && (currentTime - lastFadeTime < fadeDebounce))
        return;

    // Already fading toward same target
    if(!targets.empty()) {
        if(targets.back().clip == target) return;
    }

    if(clip == target && targets.empty()) return;

    // Log fade request
    lastFadeTime = currentTime;
    lastFadeTarget = target;

    targets.push_back(CrossFadeTarget(target, skeleton.GetRestPose(), fadeTime));
}

void CrossFadeController::Update(float dt){
    internalTime += dt;
    if(clip == nullptr || !wasSkeletonSet) return;

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

    if(stopNoLoopAnim && clip->GetLooping() == false && time >= clip->GetEndTime()){
        clip = nullptr;
    }
}

void CrossFadeController::Update(float dt, Pose& pose){
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