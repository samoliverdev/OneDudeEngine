#include "AssetManager.h"

namespace OD{

Ref<Texture2D> AssetManager::LoadTexture2D(const std::string& filePath, TextureFilter filter, bool mipmap){
    if(_textures.count(filePath)) return _textures[filePath];

    LogInfo("LoadTexture2D: %s", filePath.c_str());

    Ref<Texture2D> tex = Texture2D::CreateFromFile(filePath.c_str(), filter, mipmap);
    _textures[filePath] = tex;
    
    return tex;
}

Ref<Shader> AssetManager::LoadShaderFromFile(const std::string& filePath){
    if(_shaders.count(filePath)) return _shaders[filePath];

    LogInfo("LoadShaderFromFile: %s", filePath.c_str());

    Ref<Shader> shader = Shader::CreateFromFile(filePath);
    _shaders[filePath] = shader;
    return shader;
}

Ref<Material> AssetManager::LoadMaterial(const std::string &path){
    if(_materials.count(path)) return _materials[path];

    LogInfo("LoadMaterial: %s", path.c_str());

    Ref<Material> material = Material::CreateFromFile(path);
    _materials[path] = material;
    return material;
}

Ref<Model> AssetManager::LoadModel(const std::string &path, Ref<Shader> customShader){
    if(_models.count(path)) return _models[path];

    LogInfo("LoadModel: %s", path.c_str());

    Ref<Model> model = Model::CreateFromFile(path, customShader);
    _models[path] = model;
    return model;
}   

Ref<Texture2D> AssetManager::LoadDefautlTexture2D(){
    return LoadTexture2D("res/Builtins/Textures/White.jpg", TextureFilter::Linear, true);
}

}