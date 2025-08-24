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
    SceneManager::Get().RegisterCoreComponent<AnimatorComponent>("AnimatorComponent", "Animation");
    SceneManager::Get().RegisterSystem<AnimatorSystem>("AnimatorSystem");
}

void AnimatorComponent::OnGui(Entity& e, Scene& scene){
    AnimatorComponent& anim = scene.GetComponent<AnimatorComponent>(e);

    int i = 0;
    for(auto& layer: anim.layers){
        std::string curAnim = layer.controller.GetCurrentClip() != nullptr ? layer.controller.GetCurrentClip()->GetName() : "None";
        ImGui::Text("Layer: %d, CurrentClip: %s, Time: %f", i, curAnim.c_str(), layer.controller.GetCurrentTime());
        i += 1;
    }

    ImGui::Checkbox("Enable", &anim.enable);

    ImGui::InputInt("ToPlay", &anim.toPlay);
}

void AnimatorComponent::Play(Clip* clip, int layer){
    if(layer < 0 && layer >= layers.size()){
        LogWarning("Try Play Invalid Layer: %d", layer);
        return;
    }

    layers[layer].controller.Play(clip);

    //controller.Play(clip);
}

void AnimatorComponent::FadeTo(Clip* target, float fadeTime, int layer){
    if(layer < 0 && layer >= layers.size()){
        LogWarning("Try FadeTo Invalid Layer: %d", layer);
        return;
    }

    if(layers[layer].controller.WasSkeletonSet() == false) return; //Info: Quick fix, maybe change later
    layers[layer].controller.FadeTo(target, fadeTime);

    /*if(controller.WasSkeletonSet() == false) return; //Info: Quick fix, maybe change later
    controller.FadeTo(target, fadeTime);*/
}

void AnimatorComponent::PushLayer(){
    layers.push_back({});
}

void AnimatorComponent::PopLayer(){
    if(layers.size() <= 1){
        LogWarning("Try to pop the main layer");
        return;
    }

    layers.pop_back();
}

AnimatorComponent::Layer& AnimatorComponent::GetLayer(int layer){
    return layers[layer];
}

int AnimatorComponent::LayerCount(){
    return layers.size();
}

AnimatorSystem::AnimatorSystem(Scene* inScene):System(inScene){}

int AnimatorSystem::Type(){ 
    return SystemType::Stand | SystemType::Animation; 
}

void AnimatorSystem::Update(){
    auto view = GetScene()->GetRegistry().view<AnimatorComponent, SkinnedModelRendererComponent>();
    for(auto [entity, anim, skinned]: view.each()){
        if(anim.toPlay >= 0 && skinned.GetModel() != nullptr){
            anim.Play(skinned.GetModel()->animationClips[anim.toPlay].get());
            anim.toPlay = -1;
        }
    }
}

void AnimatorSystem::AnimationUpdate(){
    #ifdef __EMSCRIPTEN__
    return;
    #endif

    OD_PROFILE_SCOPE("AnimatorSystem::Update");

    auto Blend = [&](AnimatorComponent::Layer& layer, Pose& in, Pose& toBlend){
        Assert(in.Size() == toBlend.Size());

        if(layer.controller.GetSkeleton().GetBindPose().Size() == layer.mask.size()){
            auto& cur = layer.controller.GetCurrentPose();
            for(int i = 0; i < cur.Size(); i++){
                in.SetLocalTransform(i, Transform::Mix(in.GetLocalTransform(i), toBlend.GetLocalTransform(i), layer.mask[i]));
            }
        } else {
            auto& cur = layer.controller.GetCurrentPose();
            for(int i = 0; i < cur.Size(); i++){
                in.SetLocalTransform(i, toBlend.GetLocalTransform(i));
            }
        }
    };

    auto HandlerAnimatorByModel = [&](SkinnedModelRendererComponent& skinned, AnimatorComponent& anim){
        Assert(anim.layers.size() >= 1);

        if(skinned.GetModel() == nullptr) return;
        if(anim.enable == false) return;

        Ref<Model> model = skinned.GetModel();

        for(auto& i: anim.layers){
            if(skinned.posePalette.size() < model->skeleton.GetRestPose().Size()) skinned.posePalette.resize(model->skeleton.GetRestPose().Size());
            if(i.controller.GetCurrentPose().Size() != model->skeleton.GetBindPose().Size()) i.controller.SetSkeleton(model->skeleton); //Info: This Can work better if the model is change

            i.controller.Update(Application::DeltaTime());
            //i.controller.GetCurrentPose().GetMatrixPalette(skinned.posePalette, model->skeleton.GetInvBindPose()); 
            //skinned.finalPose = i.controller.GetCurrentPose();
        }

        int i = 0;
        for(auto& layer: anim.layers){
            if(i == 0){
                skinned.finalPose = layer.controller.GetCurrentPose();
            } else {
                if(layer.controller.GetCurrentClip() != nullptr || layer.blendIfClipIsNull) 
                    Blend(layer, skinned.finalPose, layer.controller.GetCurrentPose());
            }
            i += 1;
        }

        skinned.finalPose.GetMatrixPalette(skinned.posePalette, model->skeleton.GetInvBindPose()); 
    };

    auto HandlerAnimatorByMesh = [&](SkinnedMeshRendererComponent& skinned, AnimatorComponent& anim){
        Assert(anim.layers.size() >= 1);

        if(skinned.mesh == nullptr) return;
        if(skinned.skeleton.GetBindPose().Size() <= 0) return;
        if(anim.enable == false) return;

        for(auto& i: anim.layers){
            if(skinned.posePalette.size() < skinned.skeleton.GetRestPose().Size()) skinned.posePalette.resize(skinned.skeleton.GetRestPose().Size());
            if(i.controller.GetCurrentPose().Size() != skinned.skeleton.GetBindPose().Size()) i.controller.SetSkeleton(skinned.skeleton); //Info: This Can work better if the model is change

            //FIXME: this probabily is not work well, make like HandlerAnimatorByModel
            i.controller.Update(Application::DeltaTime());
            i.controller.GetCurrentPose().GetMatrixPalette(skinned.posePalette, skinned.skeleton.GetInvBindPose()); 
            skinned.finalPose = i.controller.GetCurrentPose();
        }
    };

    auto view = GetScene()->GetRegistry().view<AnimatorComponent, SkinnedModelRendererComponent>();
    auto view2 = GetScene()->GetRegistry().view<AnimatorComponent, SkinnedMeshRendererComponent>();

    #if InternalSystemsMulthread
        scene->GetTaskflow().emplace([=](tf::Subflow& subflow){
            for(auto [entity, anim, skinned]: view.each()){
                subflow.emplace([&](){ HandlerAnimatorByModel(skinned, anim); });
            }
        });

        for(auto e: view2){
            AnimatorComponent& anim = view2.get<AnimatorComponent>(e);
            SkinnedMeshRendererComponent& skinned = view2.get<SkinnedMeshRendererComponent>(e);
            scene->GetTaskflow().emplace([&](){ HandlerAnimatorByMesh(skinned, anim); });
        }
    #else 
        for(auto [entity, anim, skinned]: view.each()){
            HandlerAnimatorByModel(skinned, anim);
        }
        for(auto e: view2){
            AnimatorComponent& anim = view2.get<AnimatorComponent>(e);
            SkinnedMeshRendererComponent& skinned = view2.get<SkinnedMeshRendererComponent>(e);
            HandlerAnimatorByMesh(skinned, anim);
        }
    #endif

    //if(InternalSystemsMulthread){
        /*scene->GetTaskflow().emplace([=](tf::Subflow& subflow){
        for(auto [entity, anim, skinned]: view.each()){
            if(skinned.GetModel() == nullptr) continue;
            if(anim.enable == false) continue;
            
            subflow.emplace([&](){ 
            Ref<Model> model = skinned.GetModel();
            if(skinned.posePalette.size() < model->skeleton.GetRestPose().Size()) skinned.posePalette.resize(model->skeleton.GetRestPose().Size());
            if(anim.controller.GetCurrentPose().Size() != model->skeleton.GetBindPose().Size()) anim.controller.SetSkeleton(model->skeleton); //Info: This Can work better if the model is change

            anim.controller.Update(Application::DeltaTime());
            anim.controller.GetCurrentPose().GetMatrixPalette(skinned.posePalette, model->skeleton.GetInvBindPose()); 
            skinned.finalPose = anim.controller.GetCurrentPose();
            });
        }*/
        /*auto view2 = GetScene()->GetRegistry().group<AnimatorComponent, SkinnedModelRendererComponent>();
        subflow.for_each(view2.begin(), view2.end(), [view2](Entity e){
            AnimatorComponent& anim = view2.get<AnimatorComponent>(e);
            SkinnedModelRendererComponent& skinned = view2.get<SkinnedModelRendererComponent>(e);
            if(skinned.GetModel() == nullptr) return;
            if(anim.enable == false) return;
        
            Ref<Model> model = skinned.GetModel();
            if(skinned.posePalette.size() < model->skeleton.GetRestPose().Size()) skinned.posePalette.resize(model->skeleton.GetRestPose().Size());
            if(anim.controller.GetCurrentPose().Size() != model->skeleton.GetBindPose().Size()) anim.controller.SetSkeleton(model->skeleton); //Info: This Can work better if the model is change

            anim.controller.Update(Application::DeltaTime());
            anim.controller.GetCurrentPose().GetMatrixPalette(skinned.posePalette, model->skeleton.GetInvBindPose()); 
            skinned.finalPose = anim.controller.GetCurrentPose();
        });*/
        //});
    //} else {
        /*for(auto [entity, anim, skinned]: view.each()){
            if(skinned.GetModel() == nullptr) continue;
            if(anim.enable == false) continue;
            
            Ref<Model> model = skinned.GetModel();
            if(skinned.posePalette.size() < model->skeleton.GetRestPose().Size()) skinned.posePalette.resize(model->skeleton.GetRestPose().Size());
            if(anim.controller.GetCurrentPose().Size() != model->skeleton.GetBindPose().Size()) anim.controller.SetSkeleton(model->skeleton); //Info: This Can work better if the model is change

            anim.controller.Update(Application::DeltaTime());
            anim.controller.GetCurrentPose().GetMatrixPalette(skinned.posePalette, model->skeleton.GetInvBindPose()); 
            skinned.finalPose = anim.controller.GetCurrentPose();
        }*/
    //}

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

    /*auto view2 = GetScene()->GetRegistry().view<AnimatorComponent, SkinnedMeshRendererComponent>();
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
    }*/

    //JobSystem::Wait();
    //executor.run(taskflow).wait(); 

    /*return;
    for(auto e: view){
        AnimatorComponent& anim = view.get<AnimatorComponent>(e);
        SkinnedModelRendererComponent& skinned = view.get<SkinnedModelRendererComponent>(e);
        if(skinned.GetModel() == nullptr) continue;

        Ref<Model> model = skinned.GetModel();
        anim.controller.GetCurrentPose().GetMatrixPalette(skinned.posePalette, model->skeleton.GetInvBindPose());
    }*/
}

}