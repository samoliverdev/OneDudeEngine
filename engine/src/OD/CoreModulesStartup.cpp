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
#include "OD/Audio/AudioClip.h"
#include "OD/Audio/AudioSystem.h"
#include "OD/Navmesh/Navmesh.h"
#include <filesystem>

namespace OD{

void CoreModulesStartup(){
    //LogInfo("CoreModulesStartup");

    AssetTypesDB::Get().RegisterAssetType<Texture2D>(".png", [](const std::string& path){ return AssetManager::Get().LoadAsset<Texture2D>(path); });
    AssetTypesDB::Get().RegisterAssetType<Texture2D>(".jpg", [](const std::string& path){ return AssetManager::Get().LoadAsset<Texture2D>(path); });

    AssetTypesDB::Get().RegisterAssetType<Material>(".material", [](const std::string& path){ return AssetManager::Get().LoadAsset<Material>(path); });

    AssetTypesDB::Get().RegisterAssetType<AudioClip>(".mp3", [](const std::string& path){ return AssetManager::Get().LoadAsset<AudioClip>(path); });
    AssetTypesDB::Get().RegisterAssetType<AudioClip>(".wav", [](const std::string& path){ return AssetManager::Get().LoadAsset<AudioClip>(path); });

    SceneManager::Get().RegisterCoreComponent<EnvironmentComponent>("EnvironmentComponent");
    SceneManager::Get().RegisterCoreComponent<CameraComponent>("CameraComponent");
    SceneManager::Get().RegisterCoreComponent<LightComponent>("LightComponent");
    SceneManager::Get().RegisterCoreComponent<MeshRendererComponent>("MeshRendererComponent");
    SceneManager::Get().RegisterCoreComponent<ModelRendererComponent>("ModelRendererComponent");
    SceneManager::Get().RegisterCoreComponent<SkinnedModelRendererComponent>("SkinnedModelRendererComponent");
    SceneManager::Get().RegisterCoreComponent<TextRendererComponent>("TextRendererComponent");
    SceneManager::Get().RegisterCoreComponent<GizmosDrawComponent>("GizmosDrawComponent");
    SceneManager::Get().RegisterSystem<StandRenderPipeline>("StandRenderPipeline");

    SceneManager::Get().RegisterCoreComponent<RigidbodyComponent>("RigidbodyComponent");
    SceneManager::Get().RegisterSystem<PhysicsSystem>("PhysicsSystem");

    SceneManager::Get().RegisterCoreComponent<ScriptComponent>("ScriptComponent");
    SceneManager::Get().RegisterSystem<ScriptSystem>("ScriptSystem");

    SceneManager::Get().RegisterCoreComponent<AnimatorComponent>("AnimatorComponent");
    SceneManager::Get().RegisterSystem<AnimatorSystem>("AnimatorSystem");

    SceneManager::Get().RegisterCoreComponent<AudioSourceComponent>("AudioSourceComponent");
    SceneManager::Get().RegisterSystem<AudioSystem>("AudioSystem");

    SceneManager::Get().RegisterSystem<NavmeshSystem>("NavmeshSystem");

    Application::AddModule(&SceneManager::Get());
}

}