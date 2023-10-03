#include "CoreModulesStartup.h"

namespace OD{

void CoreModulesStartup(){
    //LogInfo("CoreModulesStartup");

    AssetTypesDB::Get().RegisterAssetType<Texture2D>(".png", [](const char* path){
        return AssetManager::Get().LoadTexture2D(path, TextureFilter::Linear, true);
    });
    
    AssetTypesDB::Get().RegisterAssetType<Texture2D>(".jpg", [](const char* path){
        return AssetManager::Get().LoadTexture2D(path, TextureFilter::Linear, true);
    });

    AssetTypesDB::Get().RegisterAssetType<Material>(".material", [](const char* path){
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