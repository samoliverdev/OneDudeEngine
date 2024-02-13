#include "Animator.h"
#include "OD/Core/Application.h"
#include "OD/RenderPipeline/ModelRendererComponent.h"

namespace OD{

void AnimatorComponent::OnGui(Entity& e){}

void AnimatorComponent::Play(Clip* clip){
    controller.Play(clip);
}

AnimatorSystem::AnimatorSystem(Scene* inScene):System(inScene){}

SystemType AnimatorSystem::Type(){ 
    return SystemType::Stand; 
}

void AnimatorSystem::Update(){
    auto view = GetScene()->GetRegistry().view<AnimatorComponent, SkinnedModelRendererComponent>();
    
    for(auto e: view){
        AnimatorComponent& anim = view.get<AnimatorComponent>(e);
        SkinnedModelRendererComponent& skinned = view.get<SkinnedModelRendererComponent>(e);

        if(skinned.GetModel() == nullptr) continue;

        Ref<Model> model = skinned.GetModel();

        if(skinned.posePalette.size() < model->skeleton.GetRestPose().Size()){
            skinned.posePalette.resize(model->skeleton.GetRestPose().Size());
        }

        if(anim.controller.WasSkeletonSet() == false){
            anim.controller.SetSkeleton(model->skeleton);
        }

        anim.controller.Update(Application::DeltaTime());
        anim.controller.GetCurrentPose().GetMatrixPalette(skinned.posePalette, model->skeleton.GetInvBindPose());
    }
}

}