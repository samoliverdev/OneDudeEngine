#include "CharacterAnimation.h"
#include "CharacterMovement.h"
#include "OD/Animation/Animator.h"

namespace Standard{

void CharacterAnimation::OnStart(){
    if(baseModel == nullptr){
        hasStarted = false;
        return;
    }

    idleAnimation = baseModel->FindClipByName("Idle").get();
    runningAnimation = baseModel->FindClipByName("Run").get();

    hasStarted = true;
}

void CharacterAnimation::OnUpdate(TransformComponent& transform, AnimatorComponent& anim, CharacterMovement& movement){
    if(baseModel == nullptr) return;

    if(movement.moveDir == Vector3Zero){
        if(anim.GetLayer(0).controller.GetCurrentClip() != idleAnimation) anim.FadeTo(idleAnimation, 0.1f);
    } else {
        if(anim.GetLayer(0).controller.GetCurrentClip() != runningAnimation) anim.FadeTo(runningAnimation, 0.1f);
    }
}

}