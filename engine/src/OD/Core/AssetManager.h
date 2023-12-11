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
        std::function<Ref<Asset>(const std::string&)> CreateFromFile;
    };

    template<typename T>
    void RegisterAssetType(std::string fileExtension){
        Assert(assetFuncs.find(fileExtension) == assetFuncs.end());

        AssetFuncs funcs;

        funcs.CreateFromFile = [](const char* path){
            return T::CreateFromFile(path);
        };
        
        assetFuncs[fileExtension] = funcs;
    }

    template<typename T>
    void RegisterAssetType(std::string fileExtension, std::function<Ref<Asset>(const std::string&)> createFromFile){
        Assert(assetFuncs.find(fileExtension) == assetFuncs.end());

        AssetFuncs funcs;
        funcs.CreateFromFile = createFromFile;
        assetFuncs[fileExtension] = funcs;
    }

    inline bool HasAssetByExtension(std::string fileExtension){
        return assetFuncs.find(fileExtension) != assetFuncs.end();
    }

    inline static AssetTypesDB& Get(){
        static AssetTypesDB global;
        return global;
    }

    std::unordered_map<std::string, AssetFuncs> assetFuncs;

private:
    AssetTypesDB(){}
};

class AssetManager{
public:
    Ref<Texture2D> LoadTexture2D(const std::string& filePath, Texture2DSetting settings); 
    Ref<Texture2D> LoadTexture2D(const std::string& filePath); 

    Ref<Shader> LoadShaderFromFile(const std::string& filepath);
    Ref<Model> LoadModel(const std::string &path, Ref<Shader> customShader = nullptr);
    Ref<Material> LoadMaterial(const std::string &path);

    Ref<Texture2D> LoadDefautlTexture2D();
    Ref<Shader> LoadErrorShader();

    inline void UnloadAll(){
        meshs.clear();
        models.clear();
        shaders.clear();
        textures.clear();
        materials.clear();
    }

    static AssetManager& Get();

private:
    std::unordered_map<std::string, Ref<Mesh>> meshs;
    std::unordered_map<std::string, Ref<Model>> models;
    std::unordered_map<std::string, Ref<Shader>> shaders;
    std::unordered_map<std::string, Ref<Texture2D>> textures;
    std::unordered_map<std::string, Ref<Material>> materials;
    
    //std::unordered_map<std::type_index, std::vector<Asset*>> _data;
};

}