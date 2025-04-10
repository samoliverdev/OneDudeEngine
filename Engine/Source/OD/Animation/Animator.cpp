#include "Animator.h"
#include "OD/Core/Application.h"
#include "OD/Core/JobSystem.h"
#include "OD/Core/Instrumentor.h"
#include "OD/Graphics/Model.h"
#include "OD/RenderPipeline/MeshRendererComponent.h"
#include "OD/RenderPipeline/ModelRendererComponent.h"
#include "OD/Scene/SceneManager.h"
#include <taskflow/taskflow.hpp> 
#include <taskflow/algorithm/for_each.hpp>

namespace OD{

void AnimatorModuleInit(){
    SceneManager::Get().RegisterCoreComponent<AnimatorComponent>("AnimatorComponent");
    SceneManager::Get().RegisterSystem<AnimatorSystem>("AnimatorSystem");
}

void AnimatorComponent::OnGui(Entity& e, Scene& scene){}

void AnimatorComponent::Play(Clip* clip){
    controller.Play(clip);
}

void AnimatorComponent::FadeTo(Clip* target, float fadeTime){
    if(controller.WasSkeletonSet() == false) return; //Info: Quick fix, maybe change later
    controller.FadeTo(target, fadeTime);
}

AnimatorSystem::AnimatorSystem(Scene* inScene):System(inScene){}

int AnimatorSystem::Type(){ 
    return SystemType::Late; 
}

void AnimatorSystem::LateUpdate(){
    #ifdef __EMSCRIPTEN__
    return;
    #endif

    OD_PROFILE_SCOPE("AnimatorSystem::Update");

    //tf::Executor executor;
    //tf::Taskflow taskflow;
    tf::Taskflow& taskflow = GetScene()->GetTaskflow();

    auto view = GetScene()->GetRegistry().view<AnimatorComponent, SkinnedModelRendererComponent>();

    if(InternalSystemsMulthread){
        scene->GetTaskflow().emplace([=](tf::Subflow& subflow){
        for(auto [entity, anim, skinned]: view.each()){
            if(skinned.GetModel() == nullptr) continue;
            if(anim.enable == false) continue;
            
            subflow.emplace([&](){ 
            Ref<Model> model = skinned.GetModel();
            if(skinned.posePalette.size() < model->skeleton.GetRestPose().Size()) skinned.posePalette.resize(model->skeleton.GetRestPose().Size());
            if(anim.controller.GetCurrentPose().Size() != model->skeleton.GetBindPose().Size()) anim.controller.SetSkeleton(model->skeleton); //Info: This Can work better if the model is change

            anim.controller.Update(Application::DeltaTime());
            skinned.finalPose = anim.controller.GetCurrentPose();
            });
        }
        });
    } else {
        for(auto [entity, anim, skinned]: view.each()){
            if(skinned.GetModel() == nullptr) continue;
            if(anim.enable == false) continue;
            
            Ref<Model> model = skinned.GetModel();
            if(skinned.posePalette.size() < model->skeleton.GetRestPose().Size()) skinned.posePalette.resize(model->skeleton.GetRestPose().Size());
            if(anim.controller.GetCurrentPose().Size() != model->skeleton.GetBindPose().Size()) anim.controller.SetSkeleton(model->skeleton); //Info: This Can work better if the model is change

            anim.controller.Update(Application::DeltaTime());
            skinned.finalPose = anim.controller.GetCurrentPose();
        }
    }

    /*#if InternalSystemsMulthread
    scene->GetTaskflow().emplace([=](tf::Subflow& subflow){
    #endif
        for(auto e: view){
            AnimatorComponent& anim = view.get<AnimatorComponent>(e);
            SkinnedModelRendererComponent& skinned = view.get<SkinnedModelRendererComponent>(e);
            if(skinned.GetModel() == nullptr) continue;
            if(anim.enable == false) continue;
            
            #if InternalSystemsMulthread
            subflow.emplace([&](){ 
            #endif
                Ref<Model> model = skinned.GetModel();
                if(skinned.posePalette.size() < model->skeleton.GetRestPose().Size()) skinned.posePalette.resize(model->skeleton.GetRestPose().Size());
                if(anim.controller.GetCurrentPose().Size() != model->skeleton.GetBindPose().Size()) anim.controller.SetSkeleton(model->skeleton); //Info: This Can work better if the model is change
    
                anim.controller.Update(Application::DeltaTime());
                skinned.finalPose = anim.controller.GetCurrentPose();
            #if InternalSystemsMulthread
            });
            #endif
        }
    #if InternalSystemsMulthread
    });
    #endif*/

    //view = GetScene()->GetRegistry().view<AnimatorComponent, SkinnedModelRendererComponent>();
    /*taskflow.for_each(view.begin(), view.end(), [&](Entity e){
        AnimatorComponent& anim = view.get<AnimatorComponent>(e);
        SkinnedModelRendererComponent& skinned = view.get<SkinnedModelRendererComponent>(e);
        if(skinned.GetModel() == nullptr) return;
        if(anim.enable == false) return;

        Ref<Model> model = skinned.GetModel();
        if(skinned.posePalette.size() < model->skeleton.GetRestPose().Size()) skinned.posePalette.resize(model->skeleton.GetRestPose().Size());
        if(anim.controller.GetCurrentPose().Size() != model->skeleton.GetBindPose().Size()) anim.controller.SetSkeleton(model->skeleton); //Info: This Can work better if the model is change

        anim.controller.Update(Application::DeltaTime());
        skinned.finalPose = anim.controller.GetCurrentPose();
    });*/

    /*for(auto e: view){
        AnimatorComponent& anim = view.get<AnimatorComponent>(e);
        SkinnedModelRendererComponent& skinned = view.get<SkinnedModelRendererComponent>(e);
        if(skinned.GetModel() == nullptr) continue;
        if(anim.enable == false) continue;

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
            //if(anim.controller.WasSkeletonSet() == false) anim.controller.SetSkeleton(model->skeleton);
            if(anim.controller.GetCurrentPose().Size() != model->skeleton.GetBindPose().Size()) anim.controller.SetSkeleton(model->skeleton); //Info: This Can work better if the model is change


            anim.controller.Update(Application::DeltaTime());
            //anim.controller.GetCurrentPose().GetMatrixPalette(skinned.posePalette, model->skeleton.GetInvBindPose()); 
            skinned.finalPose = anim.controller.GetCurrentPose();
        });
    }*/

    auto view2 = GetScene()->GetRegistry().view<AnimatorComponent, SkinnedMeshRendererComponent>();
    for(auto e: view2){
        AnimatorComponent& anim = view2.get<AnimatorComponent>(e);
        SkinnedMeshRendererComponent& skinned = view2.get<SkinnedMeshRendererComponent>(e);
        if(skinned.mesh == nullptr) continue;
        if(skinned.skeleton.GetBindPose().Size() <= 0) continue;
        if(anim.enable == false) continue;
        
        #if InternalSystemsMulthread
        taskflow.emplace([&](){ 
        #endif
            if(skinned.posePalette.size() < skinned.skeleton.GetRestPose().Size()) skinned.posePalette.resize(skinned.skeleton.GetRestPose().Size());
            //if(anim.controller.WasSkeletonSet() == false) anim.controller.SetSkeleton(model->skeleton);
            if(anim.controller.GetCurrentPose().Size() != skinned.skeleton.GetBindPose().Size()) anim.controller.SetSkeleton(skinned.skeleton); //Info: This Can work better if the model is change


            anim.controller.Update(Application::DeltaTime());
            //anim.controller.GetCurrentPose().GetMatrixPalette(skinned.posePalette, model->skeleton.GetInvBindPose()); 
            skinned.finalPose = anim.controller.GetCurrentPose();
        #if InternalSystemsMulthread
        });
        #endif
    }

    //JobSystem::Wait();
    //executor.run(taskflow).wait(); 

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