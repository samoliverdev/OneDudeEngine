#pragma once
#pragma once
#include "OD/Base.h"
#include "OD/Serialization/Serialization.h"
#include "OD/Utils/Allocators.h"
#include "Resource.h"
#include "ResourceAllocator.h"
#include "Package.h"
//#include <entt/entt.hpp>
#include <filesystem>
#include <mutex>
#include <efsw/efsw.hpp>

namespace OD{

#define USE_EXPERIMENTAL_ALLOCATOR
#define USE_WEAK_PTR

class OD_API ResourceTypesDB{
public:
    struct ResourceFuncs{
        std::function<Ref<Resource>()> Create;
        std::function<Ref<Resource>(const std::string&)> CreateFromFile;
    };

    template<typename T>
    void RegisterAssetType(std::string fileExtension){
        Assert(assetFuncs.find(fileExtension) == assetFuncs.end());

        ResourceFuncs funcs;

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
    void RegisterAssetType(std::string fileExtension, std::function<Ref<Resource>(const std::string&)> createFromFile){
        Assert(assetFuncs.find(fileExtension) == assetFuncs.end());

        ResourceFuncs funcs;
        funcs.CreateFromFile = createFromFile;
        funcs.Create = [](){
            return CreateRef<T>();
        };

        assetFuncs[fileExtension] = funcs;
        assetFuncsTypes[GetType<T>()] = funcs;
    }

    bool HasAssetByExtension(std::string fileExtension);

    static ResourceTypesDB& Get();

    std::unordered_map<std::string, ResourceFuncs> assetFuncs;
    std::unordered_map<Type, ResourceFuncs> assetFuncsTypes;

private:
    ResourceTypesDB(){}
};

class ResourceManagerFileUpdateListener;

class OD_API ResourceManager{
    friend class ResourceManagerFileUpdateListener;
public:
    template<class T, typename ... Args> Ref<T> LoadByPath(const std::string& path, Args&& ... args);
    //TODO: Deprectate this later
    template<class T, typename ... Args> void AddByPath(const std::string& path, Ref<T> asset);
    template<class T> bool HasByPath(const std::string& path) const;

    template<class T, typename ... Args> Ref<T> Create(Args&& ... args);
    template<typename T> IResourceView<T>* GetAllocatorView();

    void UnloadAll();
    static ResourceManager& Get();

    void StartHotReload();
    void StopHotReload();
    void ApplyHotReload();

    void Mount(Package* p);
    void UnMount(Package* p);

    inline const std::vector<Package*>& GetPackages(){ return packages; }

private:
    //std::unordered_map<std::type_index, std::unordered_map<std::string, Ref<Asset>>> data;
    //std::unordered_map<entt::id_type, std::unordered_map<std::string, Ref<Asset>>> data;
    #ifdef USE_WEAK_PTR
    std::unordered_map<Type, std::unordered_map<std::string, WeakRef<Resource>>> data;
    #else
    std::unordered_map<Type, std::unordered_map<std::string, Ref<Asset>>> data;
    #endif
    
    #ifdef USE_EXPERIMENTAL_ALLOCATOR
    std::unordered_map<Type, void*> allocator;
    #endif

    #ifdef USE_WEAK_PTR
    std::unordered_map<std::string, WeakRef<Resource>>& GetDB(Type id);
    #else
    std::unordered_map<std::string, Ref<Asset>>& GetDB(Type id);
    #endif

    efsw::FileWatcher* fileWatcher;
    ResourceManagerFileUpdateListener* listener;
    std::unordered_set<Ref<Resource>> toApplyHotReload;
    std::mutex toApplyHotReloadMutex;

    std::vector<Package*> packages;
};

template<class T>
struct OD_API ResourceRefSerialize{
    Ref<T>& asset;
    bool dontTryLoadFromMemory = true;

    ResourceRefSerialize(Ref<T>& inAsset):asset(inAsset){}

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
        if(dontTryLoadFromMemory && (path.empty() || path[0] == '#')){
            return;
        }
        /*if(path == "Memory"){
            ArchiveDumpNVP(ar, *asset);
            return;
        }*/

        asset = ResourceManager::Get().LoadByPath<T>(path); 
    }
};

template<class T>
struct OD_API ResourceVectorRefSerialize{
    std::vector<Ref<T>>& assets;

    ResourceVectorRefSerialize(std::vector<Ref<T>>&  inAssets):assets(inAssets){}

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
            assets.push_back(isNull[i] ? nullptr : ResourceManager::Get().LoadByPath<T>(path[i])); 
        }
    }
};

}

namespace OD{

template<typename T>
IResourceView<T>* ResourceManager::GetAllocatorView(){
    /*auto it = allocator.find(GetType<T>());
    if(it == allocator.end()) return nullptr;
    return reinterpret_cast<IResourceView<T>*>(it->second);*/

    Type type = GetType<T>();
    void*& entry = allocator[type];

    if(entry == nullptr){
        auto* alloc = new ResourceAllocator<T>();
        alloc->Init(1000);
        entry = alloc;
    }

    return static_cast<IResourceView<T>*>(entry);
}

template<class T, typename ... Args>
Ref<T> ResourceManager::LoadByPath(const std::string& path, Args&& ... args){
    static_assert(std::is_base_of_v<Resource, T>, "T must be derived from Asset");

    std::string resolvedPath = path;

    //auto& db = GetDB(std::type_index(typeid(T))); 
    //auto& db = data[std::type_index(typeid(T))];
    //auto& db = data[entt::type_hash<T>::value()];
    //auto& db = GetDB(entt::type_hash<T>::value()); 
    auto& db = GetDB(GetType<T>()); 

    //if(db.count(path)) return reinterpret_cast<const Ref<T>&>(db[path]);
    //if(db.count(path)) return std::dynamic_pointer_cast<T>(db[path]);
    //if(db.count(resolvedPath)) return std::static_pointer_cast<T>(db[resolvedPath]);

    #ifdef USE_WEAK_PTR
    if(db.count(resolvedPath)){
        if(auto existing = db[resolvedPath].lock()){
            return std::static_pointer_cast<T>(existing);
        }
        db.erase(resolvedPath);// expired → remove stale entry
    }
    #else
    if(db.count(resolvedPath)) return std::static_pointer_cast<T>(db[resolvedPath]);
    #endif

    //Find full path if arg path has not ext ex: "Engine/Textures/White" -> ("Engine/Textures/White.png" | "Engine/Textures/White.texturebin" | ...) 
    namespace fs = std::filesystem;
    fs::path p(path);
    bool hasExtension = p.has_extension();
    if(!hasExtension){
        T asset;
        auto extensions = asset.GetFileAssociations();

        bool find = false;

        for(const auto& ext : extensions){
            std::string testPath = path + ext;

            //Check packages
            for(auto& pkg : packages){
                if(pkg->HasFile(testPath.c_str())){
                    resolvedPath = testPath;
                    find = true;
                    break;
                }
            }

            //Check filesystem
            if(std::filesystem::exists(testPath)){
                resolvedPath = testPath;
                find = true;
                break;
            }
        }

        if(find == false){
            LogError("Asset not found with any supported extension: {}", path);
            return nullptr;
        }
    }

    //LogInfo("LoadAsset: {}", path);

    #ifdef USE_EXPERIMENTAL_ALLOCATOR
    //INFO: Add Experimental Alloctor
    /*void* allo = allocator[GetType<T>()];
    if(allo == nullptr){
        allo = new ArenaLinearAllocator<T>();
        reinterpret_cast<ArenaLinearAllocator<T>*>(allo)->Init(1000);
    }
    ArenaLinearAllocator<T>* alloc = reinterpret_cast<ArenaLinearAllocator<T>*>(allo);
    Ref<T> asset = alloc->AllocShared(std::forward<Args>(args)...);*/

    void*& allo = allocator[GetType<T>()];
    if(allo == nullptr){
        allo = new ResourceAllocator<T>();
        reinterpret_cast<ResourceAllocator<T>*>(allo)->Init(1000);
    }
    ResourceAllocator<T>* alloc = reinterpret_cast<ResourceAllocator<T>*>(allo);
    Ref<T> asset = alloc->AllocShared(std::forward<Args>(args)...);
    #else

    Assert(AssetTypesDB::Get().assetFuncsTypes.count(GetType<T>()));
    Ref<Asset> asset = AssetTypesDB::Get().assetFuncsTypes[GetType<T>()].Create();

    //Ref<T> asset = CreateRef<T>(std::forward<Args>(args)...);
    #endif

    bool hasLoadedFromPackage = false;
    for(int i = 0; i < packages.size(); i++){
        if(packages[i]->HasFile(resolvedPath.c_str())){
            if(asset->LoadFromPackage(resolvedPath, *packages[i])){
                hasLoadedFromPackage = true;
                break;
            }// else {
            //    return nullptr; //INFO: Maybe force nullptr if asset cold not load from path
            //}
        }
    }

    if(hasLoadedFromPackage == false){
        if(asset->LoadFromFile(resolvedPath) == false) return nullptr;
    }
    
    #ifdef USE_WEAK_PTR
    db[resolvedPath] = std::static_pointer_cast<Resource>(asset); //asset;
    #else
    db[resolvedPath] = asset;
    #endif
    
    //return reinterpret_cast<const Ref<T>&>(asset);
    return std::static_pointer_cast<T>(asset);
    //return asset;
}

template<class T, typename ... Args>
Ref<T> ResourceManager::Create(Args&& ... args){
    static_assert(std::is_base_of_v<Resource, T>, "T must be derived from Asset");

    return nullptr;

    Type type = GetType<T>();
    auto& db = GetDB(type);

    Ref<T> asset = nullptr;

    #ifdef USE_EXPERIMENTAL_ALLOCATOR
    void*& allo = allocator[type];
    if(allo == nullptr){
        allo = new ResourceAllocator<T>();
        reinterpret_cast<ResourceAllocator<T>*>(allo)->Init(1000);
    }

    ResourceAllocator<T>* alloc = reinterpret_cast<ResourceAllocator<T>*>(allo);
    asset = alloc->AllocShared(std::forward<Args>(args)...);
    #else
    Assert(AssetTypesDB::Get().assetFuncsTypes.count(type));
    asset = std::static_pointer_cast<T>(
        AssetTypesDB::Get().assetFuncsTypes[type].Create()
    );
    #endif

    // Mark as memory-only asset
    asset->SetPath("#Memory"); // or "" if you prefer

    // Optional: store with unique key (avoid collisions)
    /*std::string key = "#Memory_" + std::to_string(reinterpret_cast<uintptr_t>(asset.get()));

    #ifdef USE_WEAK_PTR
    db[key] = std::static_pointer_cast<Asset>(asset);
    #else
    db[key] = asset;
    #endif*/

    return asset;
}

template<class T, typename ... Args>
void ResourceManager::AddByPath(const std::string& path, Ref<T> asset){
    static_assert(std::is_base_of_v<Resource, T>, "T must be derived from Asset");

    //auto& db = data[std::type_index(typeid(T))];
    //auto& db = data[entt::type_hash<T>::value()];
    //auto& db = GetDB(entt::type_hash<T>::value()); 
    auto& db = GetDB(GetType<T>()); 

    LogInfo("AddAsset: {}", path);
    #ifdef USE_WEAK_PTR
    db[path] = std::static_pointer_cast<Resource>(asset); //asset;
    #else
    db[path] = asset;
    #endif
}

template<class T>
bool ResourceManager::HasByPath(const std::string& path) const {
    static_assert(std::is_base_of_v<Resource, T>, "T must derive from Asset");

    Type type = GetType<T>();
    auto& dbs = GetDB(type); 

    auto itDB = dbs.find(type);
    if(itDB == dbs.end()) return false;

    const auto& db = itDB->second;

    auto it = db.find(path);
    if(it == db.end()) return false;

    #ifdef USE_WEAK_PTR
    return !it->second.expired();
    #else
    return it->second != nullptr;
    #endif
}

}