#include "CrossFadeController.h"
#include "Blending.h"

namespace OD{

CrossFadeController::CrossFadeController(){
    _clip = 0;
    _time = 0.0f;
    _wasSkeletonSet = false;
}

CrossFadeController::CrossFadeController(Skeleton& skeleton){
    _clip = 0;
    _time = 0.0f;
    SetSkeleton(skeleton);
}

void CrossFadeController::SetSkeleton(Skeleton& skeleton){
    _skeleton = skeleton;
    _pose = _skeleton.GetRestPose();
    _wasSkeletonSet = true;
}

void CrossFadeController::Play(Clip* target){
    _targets.clear();
    _clip = target;
    _pose = _skeleton.GetRestPose();
    _time = target->GetStartTime();
}

void CrossFadeController::FadeTo(Clip* target, float fadeTime){
    if(_clip == 0){
        Play(target);
        return;
    }

    if(_targets.size() >= 1){
        //Clip* clip = _targets[_targets.size()-1].clip;
        //if(clip == target) return;
        if(_targets[_targets.size()-1].clip == target) return;
    } else {
        if(_clip == target) return;
    }

    _targets.push_back(CrossFadeTarget(target, _skeleton.GetRestPose(), fadeTime));
}

void CrossFadeController::Update(float dt){
    if(_clip == 0 || !_wasSkeletonSet) return;

    unsigned int numTargets = _targets.size();
    for(unsigned int i = 0; i < numTargets; i++){
        if(_targets[i].elapsed >= _targets[i].duration){
            _clip = _targets[i].clip;
            _time = _targets[i].time;
            _pose = _targets[i].pose;
            _targets.erase(_targets.begin() + i);
            break;
        }
    }

    numTargets = _targets.size();
    _pose = _skeleton.GetRestPose();
    _time = _clip->Sample(_pose, _time + dt);

    for(unsigned int i = 0; i < numTargets; i++){
        CrossFadeTarget& target = _targets[i];
        target.time = target.clip->Sample(target.pose, target.time + dt);
        target.elapsed += dt;
        float t = target.elapsed / target.duration;
        if(t > 1.0f){ t = 1.0f; }
        Blend(_pose, _pose, target.pose, t, -1);
    }
}

Pose& CrossFadeController::GetCurrentPose(){
    return _pose;
}

Clip* CrossFadeController::GetCurrentClip(){
    return _clip;
}

}