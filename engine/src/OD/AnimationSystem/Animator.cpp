#include "Animator.h"
#include "OD/RendererSystem/MeshRendererComponent.h"
#include "OD/Core/JobSystem.h"

namespace OD{

void AnimatorComponent::Serialize(YAML::Emitter& out, Entity& e){

}

void AnimatorComponent::Deserialize(YAML::Node& in, Entity& e){

}

void AnimatorComponent::OnGui(Entity& e){

}

void AnimatorComponent::Play(Animation* animation){
    if(anim == nullptr){
        anim = new Animator(animation);
    } else {
        anim->PlayAnimation(animation);
    }
}

void AnimatorSystem::Init(Scene* scene){
    _scene = scene;
}

void AnimatorSystem::Update(){
    auto view = scene()->GetRegistry().view<AnimatorComponent, MeshRendererComponent>();


#if 0
    for(auto e: view){
        AnimatorComponent& anim = view.get<AnimatorComponent>(e);
        MeshRendererComponent& renderer = view.get<MeshRendererComponent>(e);

        if(anim.anim == nullptr) continue;
        if(renderer.model() == nullptr) continue;

        anim.anim->UpdateAnimation(Application::deltaTime());

        auto transforms = anim.anim->finalBoneMatrices();
        for (int i = 0; i < transforms.size(); ++i){
            std::string s = "finalBonesMatrices[" + std::to_string(i) + "]";
            for(auto j: renderer.model()->materials){
                j->BindGlobal();
                j->SetGlobalMatrix4(s.c_str(), transforms[i]);
            }
        }
    }
#else
    for(auto e: view){
        AnimatorComponent& anim = view.get<AnimatorComponent>(e);
        MeshRendererComponent& renderer = view.get<MeshRendererComponent>(e);

        if(anim.anim == nullptr) continue;
        if(renderer.model() == nullptr) continue;

        JobSystem::Execute([&]{ 
            anim.anim->UpdateAnimation(Application::deltaTime());
        });
    }
    JobSystem::Wait();

    for(auto e: view){
        AnimatorComponent& anim = view.get<AnimatorComponent>(e);
        MeshRendererComponent& renderer = view.get<MeshRendererComponent>(e);

        if(anim.anim == nullptr) continue;
        if(renderer.model() == nullptr) continue;

        auto transforms = anim.anim->finalBoneMatrices();
        for (int i = 0; i < transforms.size(); ++i){
            std::string s = "finalBonesMatrices[" + std::to_string(i) + "]";
            for(auto j: renderer.model()->materials){
                j->shader()->Bind();
                j->shader()->SetMatrix4(s.c_str(), transforms[i]);
            }
        }
    }   
#endif

    
}

}