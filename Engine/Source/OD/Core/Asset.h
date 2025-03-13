#pragma once
#include "OD/Defines.h"
#include "OD/Base.h"
#include "OD/Serialization/Serialization.h"

namespace OD{

class OD_API Asset{
public:
    virtual ~Asset() = default; //virtual ~Asset(){}
    virtual std::string& Path();
    virtual void OnGui(){}
    virtual void Reload(){ LoadFromFile(path); }
    virtual void Save(){}
    virtual bool LoadFromFile(const std::string& path);
    virtual std::vector<std::string> GetFileAssociations();
    bool HasFileExtension(const std::string& fileExtension);

protected:
    std::string path = "Memory";
};

class OD_API AssetTypesDB{
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

    bool HasAssetByExtension(std::string fileExtension);

    static AssetTypesDB& Get();

    std::unordered_map<std::string, AssetFuncs> assetFuncs;

private:
    AssetTypesDB(){}
};

class OD_API AssetManager{
public:
    template<class T, typename ... Args> Ref<T> LoadAsset(const std::string& path, Args&& ... args);
    template<class T, typename ... Args> void AddAsset(const std::string& path, Ref<T> asset);
    void UnloadAll();
    static AssetManager& Get();

private:
    std::unordered_map<std::type_index, std::unordered_map<std::string, Ref<Asset>>> data;
};

template<class T>
struct OD_API AssetRefSerialize{
    Ref<T>& asset;

    AssetRefSerialize(Ref<T>& inAsset):asset(inAsset){}

    template<class Archive>
    void save(Archive& ar) const{
        bool isNull = asset == nullptr;
        std::string path = isNull == false ? asset->Path() : "";
        ArchiveDumpNVP(ar, isNull);
        ArchiveDumpNVP(ar, path);
        /*if(path == "Memory"){
            ArchiveDumpNVP(ar, *asset);
            return;
        }*/
    }

    template<class Archive>
    void load(Archive& ar){
        bool isNull;
        std::string path;
        ArchiveDumpNVP(ar, isNull);
        ArchiveDumpNVP(ar, path);

        if(isNull){
            asset = nullptr;
            return;
        }
        /*if(path == "Memory"){
            ArchiveDumpNVP(ar, *asset);
            return;
        }*/

        asset = AssetManager::Get().LoadAsset<T>(path); 
    }
};

template<class T>
struct OD_API AssetVectorRefSerialize{
    std::vector<Ref<T>>& assets;

    AssetVectorRefSerialize(std::vector<Ref<T>>&  inAssets):assets(inAssets){}

    template<class Archive>
    void save(Archive& ar) const{
        std::vector<bool> isNull;
        std::vector<std::string> path;

        for(auto& asset: assets){
            isNull.push_back(asset == nullptr);
            path.push_back(isNull[isNull.size()-1] == false ? asset->Path() : "");
        }

        ArchiveDumpNVP(ar, isNull);
        ArchiveDumpNVP(ar, path);
    }

    template<class Archive>
    void load(Archive& ar){
        assets.clear();

        std::vector<bool> isNull;
        std::vector<std::string> path;

        ArchiveDumpNVP(ar, isNull);
        ArchiveDumpNVP(ar, path);

        for(int i = 0; i < isNull.size(); i++){
            assets.push_back(isNull[i] ? nullptr : AssetManager::Get().LoadAsset<T>(path[i])); 
        }
    }
};

}

namespace OD{

template<class T, typename ... Args>
Ref<T> AssetManager::LoadAsset(const std::string& path, Args&& ... args){
    static_assert(std::is_base_of_v<Asset, T>, "T must be derived from Asset");

    auto& db = data[std::type_index(typeid(T))];
    //if(db.count(path)) return reinterpret_cast<const Ref<T>&>(db[path]);
    //if(db.count(path)) return std::dynamic_pointer_cast<T>(db[path]);
    if(db.count(path)) return std::static_pointer_cast<T>(db[path]);

    LogInfo("LoadAsset: %s", path.c_str());
    Ref<T> asset = CreateRef<T>(std::forward<Args>(args)...);
    asset->LoadFromFile(path);
    db[path] = asset;
    
    //return reinterpret_cast<const Ref<T>&>(d);
    //return std::static_pointer_cast<T>(d);
    return asset;
}

template<class T, typename ... Args>
void AssetManager::AddAsset(const std::string& path, Ref<T> asset){
    static_assert(std::is_base_of_v<Asset, T>, "T must be derived from Asset");

    auto& db = data[std::type_index(typeid(T))];
    LogInfo("AddAsset: %s", path.c_str());
    db[path] = asset;
}

}