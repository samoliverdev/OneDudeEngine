#pragma once
#include "OD/Defines.h"
#include "OD/Base.h"
#include "OD/Serialization/Serialization.h"
#include <entt/entt.hpp>
#include <mutex>

#include "OD/Utils/Allocators.h"

#include <efsw/efsw.hpp>

namespace OD{

//#define USE_EXPERIMENTAL_ALLOCATOR

class OD_API Asset{
public:
    virtual ~Asset() = default; //virtual ~Asset(){}
    virtual std::string& Path();
    virtual void OnGui(){}
    virtual void Reload(){ LoadFromFile(path); }
    virtual void Save(){}//INFO: Maybe remove later
    virtual bool SaveAs(const std::string& path){ return false; }//INFO: Maybe Rename
    virtual bool LoadFromFile(const std::string& path);
    virtual std::vector<std::string> GetFileAssociations();
    bool HasFileExtension(const std::string& fileExtension);

protected:
    std::string path = "Memory";
};

class OD_API AssetTypesDB{
public:
    struct AssetFuncs{
        std::function<Ref<Asset>()> Create;
        std::function<Ref<Asset>(const std::string&)> CreateFromFile;
    };

    template<typename T>
    void RegisterAssetType(std::string fileExtension){
        Assert(assetFuncs.find(fileExtension) == assetFuncs.end());

        AssetFuncs funcs;

        funcs.Create = [](){
            return CreateRef<T>();
        };

        funcs.CreateFromFile = [](const char* path){
            return T::CreateFromFile(path);
        };
        
        assetFuncs[fileExtension] = funcs;
        assetFuncsTypes[GetType<T>()] = funcs;
    }

    template<typename T>
    void RegisterAssetType(std::string fileExtension, std::function<Ref<Asset>(const std::string&)> createFromFile){
        Assert(assetFuncs.find(fileExtension) == assetFuncs.end());

        AssetFuncs funcs;
        funcs.CreateFromFile = createFromFile;
        funcs.Create = [](){
            return CreateRef<T>();
        };

        assetFuncs[fileExtension] = funcs;
        assetFuncsTypes[GetType<T>()] = funcs;
    }

    bool HasAssetByExtension(std::string fileExtension);

    static AssetTypesDB& Get();

    std::unordered_map<std::string, AssetFuncs> assetFuncs;
    std::unordered_map<Type, AssetFuncs> assetFuncsTypes;

private:
    AssetTypesDB(){}
};

class AssetManagerFileUpdateListener;

class OD_API AssetManager{
    friend class AssetManagerFileUpdateListener;
public:
    template<class T, typename ... Args> Ref<T> LoadAsset(const std::string& path, Args&& ... args);
    template<class T, typename ... Args> void AddAsset(const std::string& path, Ref<T> asset);
    void UnloadAll();
    static AssetManager& Get();

    void StartHotReload();
    void StopHotReload();
    void ApplyHotReload();

private:
    //std::unordered_map<std::type_index, std::unordered_map<std::string, Ref<Asset>>> data;
    //std::unordered_map<entt::id_type, std::unordered_map<std::string, Ref<Asset>>> data;
    std::unordered_map<Type, std::unordered_map<std::string, Ref<Asset>>> data;
    
    #ifdef USE_EXPERIMENTAL_ALLOCATOR
    std::unordered_map<Type, void*> allocator;
    #endif

    std::unordered_map<std::string, Ref<Asset>>& GetDB(Type id);

    efsw::FileWatcher* fileWatcher;
    AssetManagerFileUpdateListener* listener;
    std::unordered_set<Ref<Asset>> toApplyHotReload;
    std::mutex toApplyHotReloadMutex;
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

    //auto& db = GetDB(std::type_index(typeid(T))); 
    //auto& db = data[std::type_index(typeid(T))];
    //auto& db = data[entt::type_hash<T>::value()];
    //auto& db = GetDB(entt::type_hash<T>::value()); 
    auto& db = GetDB(GetType<T>()); 

    //if(db.count(path)) return reinterpret_cast<const Ref<T>&>(db[path]);
    //if(db.count(path)) return std::dynamic_pointer_cast<T>(db[path]);
    if(db.count(path)) return std::static_pointer_cast<T>(db[path]);

    LogInfo("LoadAsset: %s", path.c_str());

    #ifdef USE_EXPERIMENTAL_ALLOCATOR
    //INFO: Add Experimental Alloctor
    void* allo = allocator[GetType<T>()];
    if(allo == nullptr){
        allo = new ArenaLinearAllocator<T>();
        reinterpret_cast<ArenaLinearAllocator<T>*>(allo)->Init(1000);
    }
    ArenaLinearAllocator<T>* alloc = reinterpret_cast<ArenaLinearAllocator<T>*>(allo);
    Ref<T> asset = alloc->AllocShared();
    #else

    Assert(AssetTypesDB::Get().assetFuncsTypes.count(GetType<T>()));
    Ref<Asset> asset = AssetTypesDB::Get().assetFuncsTypes[GetType<T>()].Create();

    //Ref<T> asset = CreateRef<T>(std::forward<Args>(args)...);
    #endif

    if(asset->LoadFromFile(path) == false) return nullptr;
    db[path] = asset;
    
    //return reinterpret_cast<const Ref<T>&>(asset);
    return std::static_pointer_cast<T>(asset);
    //return asset;
}

template<class T, typename ... Args>
void AssetManager::AddAsset(const std::string& path, Ref<T> asset){
    static_assert(std::is_base_of_v<Asset, T>, "T must be derived from Asset");

    //auto& db = data[std::type_index(typeid(T))];
    //auto& db = data[entt::type_hash<T>::value()];
    //auto& db = GetDB(entt::type_hash<T>::value()); 
    auto& db = GetDB(GetType<T>()); 

    LogInfo("AddAsset: %s", path.c_str());
    db[path] = asset;
}

}