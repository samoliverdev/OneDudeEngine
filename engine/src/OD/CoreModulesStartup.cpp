#include "CoreModulesStartup.h"
#include <filesystem>

namespace OD{

void CoreModulesStartup(){
    //LogInfo("CoreModulesStartup");

    AssetTypesDB::Get().RegisterAssetType<Texture2D>(".png", [](const std::string& path){
        return AssetManager::Get().LoadTexture2D(path);
    });
    
    AssetTypesDB::Get().RegisterAssetType<Texture2D>(".jpg", [](const std::string& path){
        return AssetManager::Get().LoadTexture2D(path);
    });

    AssetTypesDB::Get().RegisterAssetType<Material>(".material", [](const std::string& path){
        return AssetManager::Get().LoadMaterial(path);
    });

    SceneManager::Get().RegisterCoreComponent<EnvironmentComponent>("EnvironmentComponent");
    SceneManager::Get().RegisterCoreComponent<CameraComponent>("CameraComponent");
    SceneManager::Get().RegisterCoreComponent<LightComponent>("LightComponent");
    SceneManager::Get().RegisterCoreComponent<MeshRendererComponent>("MeshRendererComponent");

    SceneManager::Get().RegisterCoreComponent<RigidbodyComponent>("RigidbodyComponent");

    SceneManager::Get().RegisterCoreComponent<AnimatorComponent>("AnimatorComponent");

    SceneManager::Get().RegisterCoreComponent<ScriptComponent>("ScriptComponent");
}

}