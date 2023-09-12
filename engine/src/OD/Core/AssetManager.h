#pragma once

#include "OD/Defines.h"
#include "Asset.h"
#include "OD/Renderer/Mesh.h"
#include "OD/Renderer/Model.h"
#include "OD/Renderer/Shader.h"
#include "OD/Renderer/Texture.h"
#include "OD/Renderer/Material.h"
#include <typeinfo>
#include <typeindex>

namespace OD{

class AssetManager{
public:
    Ref<Texture2D> LoadTexture2D(const std::string& filePath, TextureFilter filter, bool mipmap); 
    Ref<Shader> LoadShaderFromFile(const std::string& filepath);
    Ref<Model> LoadModel(const std::string &path, Ref<Shader> customShader = nullptr);
    Ref<Material> LoadMaterial(const std::string &path);

    inline void UnloadAll(){
        _meshs.clear();
        _models.clear();
        _shaders.clear();
        _textures.clear();
        _materials.clear();
    }

    inline static AssetManager& Get(){
        static AssetManager global;
        return global;
    }

private:
    std::unordered_map<std::string, Ref<Mesh>> _meshs;
    std::unordered_map<std::string, Ref<Model>> _models;
    std::unordered_map<std::string, Ref<Shader>> _shaders;
    std::unordered_map<std::string, Ref<Texture2D>> _textures;
    std::unordered_map<std::string, Ref<Material>> _materials;
    //std::unordered_map<std::type_index, std::vector<Asset*>> _data;
};

}