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
#include <functional>
#include <unordered_map>

namespace OD{

class AssetTypesDB{
public:
    struct AssetFuncs{
        std::function<Ref<Asset>(const char*)> CreateFromFile;
    };

    template<typename T>
    void RegisterAssetType(std::string fileExtension){
        Assert(_assetFuncs.find(fileExtension) == _assetFuncs.end());

        AssetFuncs funcs;

        funcs.CreateFromFile = [](const char* path){
            return T::CreateFromFile(path);
        };
        
        _assetFuncs[fileExtension] = funcs;
    }

    inline bool HasAssetByExtension(std::string fileExtension){
        return _assetFuncs.find(fileExtension) != _assetFuncs.end();
    }

    inline static AssetTypesDB& Get(){
        static AssetTypesDB global;
        return global;
    }

    inline static void _Init(){
        AssetTypesDB::Get().RegisterAssetType<Texture2D>(".png");
        AssetTypesDB::Get().RegisterAssetType<Texture2D>(".jpg");
        AssetTypesDB::Get().RegisterAssetType<Material>(".material");
    }

    std::unordered_map<std::string, AssetFuncs> _assetFuncs;
};

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