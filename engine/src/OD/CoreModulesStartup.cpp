#include "CoreModulesStartup.h"
#include "OD/Core/Application.h"
#include "OD/Core/Asset.h"
#include "OD/Scene/Scene.h"
#include "OD/Scene/SceneManager.h"
#include "OD/Scene/Scripts.h"
#include "OD/Animation/Animator.h"
#include "OD/Physics/PhysicsSystem.h"
#include "OD/RenderPipeline/MeshRendererComponent.h"
#include "OD/RenderPipeline/ModelRendererComponent.h"
#include "OD/RenderPipeline/TextRendererComponent.h"
#include "OD/RenderPipeline/StandRenderPipeline.h"
#include <filesystem>

namespace OD{

void CoreModulesStartup(){
    //LogInfo("CoreModulesStartup");

    AssetTypesDB::Get().RegisterAssetType<Texture2D>(".png", [](const std::string& path){
        //return AssetManager::Get().LoadTexture2D(path);
        return AssetManager::Get().LoadAsset<Texture2D>(path);
    });
    
    AssetTypesDB::Get().RegisterAssetType<Texture2D>(".jpg", [](const std::string& path){
        //return AssetManager::Get().LoadTexture2D(path);
        return AssetManager::Get().LoadAsset<Texture2D>(path);
    });

    AssetTypesDB::Get().RegisterAssetType<Material>(".material", [](const std::string& path){
        //return AssetManager::Get().LoadMaterial(path);
        return AssetManager::Get().LoadAsset<Material>(path);
    });

    SceneManager::Get().RegisterCoreComponent<EnvironmentComponent>("EnvironmentComponent");
    SceneManager::Get().RegisterCoreComponent<CameraComponent>("CameraComponent");
    SceneManager::Get().RegisterCoreComponent<LightComponent>("LightComponent");
    SceneManager::Get().RegisterCoreComponent<MeshRendererComponent>("MeshRendererComponent");
    SceneManager::Get().RegisterCoreComponent<ModelRendererComponent>("ModelRendererComponent");
    SceneManager::Get().RegisterCoreComponent<SkinnedModelRendererComponent>("SkinnedModelRendererComponent");
    SceneManager::Get().RegisterCoreComponent<TextRendererComponent>("TextRendererComponent");
    SceneManager::Get().RegisterSystem<StandRenderPipeline>("StandRenderPipeline");

    SceneManager::Get().RegisterCoreComponent<JointComponent>("JointComponent");
    SceneManager::Get().RegisterCoreComponent<RigidbodyComponent>("RigidbodyComponent");
    SceneManager::Get().RegisterCoreComponent<RagdollComponent>("RagdollComponent");
    SceneManager::Get().RegisterSystem<PhysicsSystem>("PhysicsSystem");

    SceneManager::Get().RegisterCoreComponent<ScriptComponent>("ScriptComponent");
    SceneManager::Get().RegisterSystem<ScriptSystem>("ScriptSystem");

    SceneManager::Get().RegisterCoreComponent<AnimatorComponent>("AnimatorComponent");
    SceneManager::Get().RegisterSystem<AnimatorSystem>("AnimatorSystem");

    Application::AddModule(&SceneManager::Get());
}

}