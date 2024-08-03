#include "Animator.h"
#include "OD/Core/Application.h"
#include "OD/RenderPipeline/ModelRendererComponent.h"
#include "OD/Core/JobSystem.h"
#include <taskflow/taskflow.hpp> 
#include "OD/Scene/SceneManager.h"

namespace OD{

void AnimatorModuleInit(){
    SceneManager::Get().RegisterCoreComponent<AnimatorComponent>("AnimatorComponent");
    SceneManager::Get().RegisterSystem<AnimatorSystem>("AnimatorSystem");
}

void AnimatorComponent::OnGui(Entity& e){}

void AnimatorComponent::Play(Clip* clip){
    controller.Play(clip);
}

void AnimatorComponent::FadeTo(Clip* target, float fadeTime){
    controller.FadeTo(target, fadeTime);
}

AnimatorSystem::AnimatorSystem(Scene* inScene):System(inScene){}

SystemType AnimatorSystem::Type(){ 
    return SystemType::Stand; 
}

void AnimatorSystem::Update(){
    auto view = GetScene()->GetRegistry().view<AnimatorComponent, SkinnedModelRendererComponent>();
    tf::Executor executor;
    tf::Taskflow taskflow;
    
    for(auto e: view){
        AnimatorComponent& anim = view.get<AnimatorComponent>(e);
        SkinnedModelRendererComponent& skinned = view.get<SkinnedModelRendererComponent>(e);
        if(skinned.GetModel() == nullptr) continue;

        //Ref<Model> model = skinned.GetModel();
        //if(skinned.posePalette.size() < model->skeleton.GetRestPose().Size()) skinned.posePalette.resize(model->skeleton.GetRestPose().Size());
        //if(anim.controller.WasSkeletonSet() == false) anim.controller.SetSkeleton(model->skeleton);

        //anim.controller.Update(Application::DeltaTime());
        //anim.controller.GetCurrentPose().GetMatrixPalette(skinned.posePalette, model->skeleton.GetInvBindPose());
        //if(skinned.skeletonEntities.size() == 0) skinned.CreateSkeletonEntites(Entity(e, GetScene()));
        //skinned.UpdateSkeletonEntites(anim.controller.GetCurrentPose());

        //JobSystem::Execute([&](){ anim.controller.Update(Application::DeltaTime()); });
        taskflow.emplace([&](){ 
            Ref<Model> model = skinned.GetModel();
            if(skinned.posePalette.size() < model->skeleton.GetRestPose().Size()) skinned.posePalette.resize(model->skeleton.GetRestPose().Size());
            if(anim.controller.WasSkeletonSet() == false) anim.controller.SetSkeleton(model->skeleton);

            anim.controller.Update(Application::DeltaTime());
            anim.controller.GetCurrentPose().GetMatrixPalette(skinned.posePalette, model->skeleton.GetInvBindPose()); 
        });
    }
    //JobSystem::Wait();
    executor.run(taskflow).wait(); 

    return;
    for(auto e: view){
        AnimatorComponent& anim = view.get<AnimatorComponent>(e);
        SkinnedModelRendererComponent& skinned = view.get<SkinnedModelRendererComponent>(e);
        if(skinned.GetModel() == nullptr) continue;

        Ref<Model> model = skinned.GetModel();
        anim.controller.GetCurrentPose().GetMatrixPalette(skinned.posePalette, model->skeleton.GetInvBindPose());
    }
}

}