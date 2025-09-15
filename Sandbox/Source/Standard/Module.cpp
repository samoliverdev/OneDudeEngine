#include "Module.h"
#include "OD/Scene/SceneManager.h"
#include "OD/Physics/PhysicsSystem.h"
#include "OD/Animation/Animator.h"
#include "Camera/FreeCamera.h"
#include "Camera/ThirdPersonCamera.h"
#include "Character/CharacterMovement.h"
#include "Character/CharacterAnimation.h"
#include "Greyboxing/Greyboxing.h"
#include "Generator/HeightmapGenerator.h"

namespace Standard{

void ModuleInit(){
    SceneManager::Get().RegisterComponent<FreeCamera>("Standard/FreeCamera", "Standard/Camera");
    SceneManager::Get().RegisterComponent<ThirdPersonCamera>("Standard/ThirdPersonCamera", "Standard");
    SceneManager::Get().RegisterComponent<CharacterMovement>("Standard/CharacterMovement", "Standard");
    SceneManager::Get().RegisterComponent<CharacterAnimation>("Standard/CharacterAnimation", "Standard");
    SceneManager::Get().RegisterComponent<Greyboxing>("Standard/Greyboxing", "Standard");
    SceneManager::Get().RegisterComponent<HeightmapGenerator>("Standard/HeightmapGenerator", "Standard");
    SceneManager::Get().RegisterSystem<StandardAssetSystem>("Standard/StandardAssetSystem");
}

StandardAssetSystem::StandardAssetSystem(Scene* inscene):System(inscene){
    defaultMaterial = CreateRef<Material>(AssetManager::Get().LoadAsset<Shader>("Engine/Shaders/Lit.glsl"));
    defaultMaterial->SetTexture("mainTex", AssetManager::Get().LoadAsset<Texture2D>("Standard/Textures/GreyboxTextures/greybox_grey_grid.png"));
}

StandardAssetSystem::~StandardAssetSystem(){

}

void StandardAssetSystem::Update(){
    auto greyboxingView = scene->GetRegistry().view<Greyboxing, TransformComponent>();
    for(auto [entity, greyboxing, trans]: greyboxingView.each()){
        if(greyboxing.isDirty == true){
            greyboxing.isDirty = false;
            greyboxing.UpdateMesh(*scene, entity, defaultMaterial);
        }
    }

    if(scene->Running() == false) return;

    auto freeCameraView = scene->GetRegistry().view<FreeCamera, TransformComponent>();
    for(auto [entity, camera, trans]: freeCameraView.each()){
        if(camera.hasStarted == false) camera.OnStart(trans);
        camera.OnUpdate(trans);
    }

    auto tpsCameraView = scene->GetRegistry().view<ThirdPersonCamera, TransformComponent>();
    for(auto [entity, camera, trans]: tpsCameraView.each()){
        if(camera.hasStarted == false) camera.OnStart();
        camera.OnUpdate(*scene, trans);
    }

    auto charMovemetView = scene->GetRegistry().view<CharacterMovement, TransformComponent, RigidbodyComponent>();
    for(auto [entity, movement, trans, rb]: charMovemetView.each()){
        if(movement.hasStarted == false) movement.OnStart(rb);
        movement.OnUpdate(*scene, trans, rb);
    }

    auto charAnimationView = scene->GetRegistry().view<CharacterAnimation, CharacterMovement, TransformComponent, AnimatorComponent>();
    for(auto [entity, charAnim, movement, trans, anim]: charAnimationView.each()){
        if(charAnim.hasStarted == false) charAnim.OnStart();
        charAnim.OnUpdate(trans, anim, movement);
    }
}

void StandardAssetSystem::LateUpdate(){
    if(scene->Running() == false) return;

    auto tpsCameraView = scene->GetRegistry().view<ThirdPersonCamera, TransformComponent>();
    for(auto [entity, camera, trans]: tpsCameraView.each()){
        if(camera.hasStarted == false) camera.OnStart();
        camera.OnUpdate(*scene, trans, false);
    }
}

}