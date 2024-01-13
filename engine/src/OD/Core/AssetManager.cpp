#include "AssetManager.h"

namespace OD{

AssetManager global;

AssetManager& AssetManager::Get(){
    return global;
}

Ref<Texture2D> AssetManager::LoadTexture2D(const std::string& filePath, Texture2DSetting settings){
    if(textures.count(filePath)) return textures[filePath];

    LogInfo("LoadTexture2D: %s", filePath.c_str());

    Ref<Texture2D> tex = Texture2D::CreateFromFile(filePath.c_str(), settings);
    Assert(tex != nullptr);

    textures[filePath] = tex;
    
    return tex;
}

Ref<Texture2D> AssetManager::LoadTexture2D(const std::string& filePath){
    if(textures.count(filePath)) return textures[filePath];

    LogInfo("LoadTexture2D: %s", filePath.c_str());

    Ref<Texture2D> tex = Texture2D::CreateFromFile(filePath.c_str(), Texture2DSetting());
    Assert(tex != nullptr);

    textures[filePath] = tex;
    
    return tex;
}

Ref<Shader> AssetManager::LoadShaderFromFile(const std::string& filePath){
    if(shaders.count(filePath)) return shaders[filePath];

    LogInfo("LoadShaderFromFile: %s", filePath.c_str());

    Ref<Shader> shader = Shader::CreateFromFile(filePath);
    Assert(shader != nullptr);

    shaders[filePath] = shader;
    return shader;
}

Ref<Material> AssetManager::LoadMaterial(const std::string &path){
    if(materials.count(path)) return materials[path];

    //LogInfo("LoadMaterial: %s Hash: %zd Size: %zd", path.c_str(), std::hash<std::string>{}(path), path.size());
    LogInfo("LoadMaterial: %s", path.c_str());

    Ref<Material> material = Material::CreateFromFile(path);
    materials[path] = material;
    return material;
}

Ref<Model> AssetManager::LoadModel(const std::string &path, Ref<Shader> customShader){
    if(models.count(path)) return models[path];

    LogInfo("LoadModel: %s", path.c_str());

    Ref<Model> model = CreateRef<Model>();
    Model::CreateFromFile(*model, path, customShader);

    models[path] = model;
    return model;
}   

Ref<Texture2D> AssetManager::LoadDefautlTexture2D(){
    return LoadTexture2D("res/Engine/Textures/White.jpg", {TextureFilter::Linear, true});
}

Ref<Shader> AssetManager::LoadErrorShader(){
    return LoadShaderFromFile("res/Engine/Shaders/Error.glsl");
}

}