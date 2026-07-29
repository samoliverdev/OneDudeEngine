#include "Module.h"
#include <OD/Scene/SceneManager.h>
#include <OD/Physics/PhysicsSystem.h>
#include <OD/Animation/Animator.h>
#include "Camera/FreeCamera.h"
#include "Camera/ThirdPersonCamera.h"
#include "Character/CharacterMovement.h"
#include "Character/CharacterAnimation.h"
#include "Greyboxing/Greyboxing.h"
#include "Generator/HeightmapGenerator.h"
#include "ParticleSystem/ParticleSystem.h"
#include "Effects/LineRenderer.h"

namespace Standard{

void ModuleInit(){
    SceneManager::Get().RegisterComponent<FreeCamera>("Standard/FreeCamera", "Standard/Camera");
    SceneManager::Get().RegisterComponent<ThirdPersonCamera>("Standard/ThirdPersonCamera", "Standard");
    SceneManager::Get().RegisterComponent<CharacterMovement>("Standard/CharacterMovement", "Standard");
    SceneManager::Get().RegisterComponent<CharacterAnimation>("Standard/CharacterAnimation", "Standard");
    SceneManager::Get().RegisterComponent<Greyboxing>("Standard/Greyboxing", "Standard");
    SceneManager::Get().RegisterComponent<HeightmapGenerator>("Standard/HeightmapGenerator", "Standard");
    SceneManager::Get().RegisterSystem<StandardAssetSystem>("Standard/StandardAssetSystem");

    RenderContext::RegisterRenderFeature<ParticleRendererFeature>();
    SceneManager::Get().RegisterComponent<ParticleComponent>("Standard/ParticleComponent", "Standard");
    SceneManager::Get().RegisterSystem<ParticleManageSystem>("Standard/ParticleManageSystem");

    SceneManager::Get().RegisterComponent<LineRenderer>("Standard/LineRenderer", "Standard");
}

StandardAssetSystem::StandardAssetSystem(){
    name = "StandardAssetSystem";
    defaultMaterial = CreateRef<Material>(ResourceManager::Get().LoadByPath<Shader>("Engine/Shaders/Lit.glsl"));
    defaultMaterial->SetTexture("mainTex", ResourceManager::Get().LoadByPath<Texture2D>("Standard/Textures/GreyboxTextures/greybox_grey_solid_2.png"));
}

StandardAssetSystem::~StandardAssetSystem(){

}

void StandardAssetSystem::Update(Scene& scene){
    auto greyboxingView = scene.GetRegistry().view<Greyboxing, TransformComponent>();
    for(auto [entity, greyboxing, trans]: greyboxingView.each()){
        if(greyboxing.isDirty == true){
            greyboxing.isDirty = false;
            greyboxing.UpdateMesh(scene, entity, defaultMaterial);
        }
    }

    auto lineRendererView = scene.GetRegistry().view<LineRenderer, TransformComponent>();
    for(auto [entity, line, trans]: lineRendererView.each()){
        if(line.isDirty == false) continue;
        line.UpdateMesh(scene, entity);
    }

    if(scene.Running() == false) return;

    auto freeCameraView = scene.GetRegistry().view<FreeCamera, TransformComponent>(entt::exclude<SelfDisable>);
    for(auto [entity, camera, trans]: freeCameraView.each()){
        if(camera.hasStarted == false) camera.OnStart(trans);
        camera.OnUpdate(trans);
    }

    auto tpsCameraView = scene.GetRegistry().view<ThirdPersonCamera, TransformComponent>();
    for(auto [entity, camera, trans]: tpsCameraView.each()){
        if(camera.hasStarted == false) camera.OnStart();
        camera.OnUpdate(scene, trans);
    }

    auto charAnimationView = scene.GetRegistry().view<CharacterAnimation, CharacterMovement, TransformComponent, AnimatorComponent>();
    for(auto [entity, charAnim, movement, trans, anim]: charAnimationView.each()){
        if(charAnim.hasStarted == false) charAnim.OnStart();
        charAnim.OnUpdate(trans, anim, movement);
    }
}

void StandardAssetSystem::FixedPhysicsUpdate(Scene& scene){
    auto charMovemetView = scene.GetRegistry().view<CharacterMovement, TransformComponent, RigidbodyComponent>();
    for(auto [entity, movement, trans, rb]: charMovemetView.each()){
        if(movement.hasStarted == false) movement.OnStart(rb);
        movement.OnFixedUpdate(scene, trans, rb);
    }
}

void StandardAssetSystem::LateUpdate(Scene& scene){
    if(scene.Running() == false) return;

    auto tpsCameraView = scene.GetRegistry().view<ThirdPersonCamera, TransformComponent>();
    for(auto [entity, camera, trans]: tpsCameraView.each()){
        if(camera.hasStarted == false) camera.OnStart();
        camera.OnUpdate(scene, trans, false);
    }
}

}