#pragma once

#include "OD/Defines.h"
#include "OD/Serialization/Serialization.h"

namespace OD{

class OD_API Asset{
public:
    virtual ~Asset(){}

    inline std::string& Path(){ return path; }
    inline void Path(const std::string& inPath){ path = inPath; }

    virtual void OnGui(){}
    virtual void Reload(){ LoadFromFile(path); }
    virtual void Save(){}
    virtual void LoadFromFile(const std::string& path){}

    inline virtual std::vector<std::string> GetFileAssociations(){ return std::vector<std::string>(); }

    inline bool HasFileExtension(const std::string& fileExtension){
        for(auto& i: GetFileAssociations()){
            if(i == fileExtension) return true;
        }
        return false;
    }

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

class OD_API AssetManager{
public:
    template<class T>
    Ref<T> LoadAsset(const std::string& path){
        auto& db = data[std::type_index(typeid(T))];
        //if(db.count(path)) return reinterpret_cast<const Ref<T>&>(db[path]);
        if(db.count(path)) return std::static_pointer_cast<T>(db[path]);

        LogInfo("LoadAsset: %s", path.c_str());

        Ref<T> d = CreateRef<T>();
        d->LoadFromFile(path);
        db[path] = d;
        
        //return reinterpret_cast<const Ref<T>&>(d);
        return std::static_pointer_cast<T>(d);
    }

    inline void UnloadAll(){
        data.clear();
    }

    static AssetManager& Get();

private:
    std::unordered_map<std::type_index, std::unordered_map<std::string, Ref<Asset>>> data;
};

template<class T>
struct OD_API AssetRef{
    Ref<T>& asset;

    AssetRef(Ref<T>& inAsset):asset(inAsset){}

    template<class Archive>
    void save(Archive& ar) const{
        bool isNull = asset == nullptr;
        std::string path = isNull == false ? asset->Path() : "";
        ArchiveDumpNVP(ar, isNull);
        ArchiveDumpNVP(ar, path);
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

        asset = AssetManager::Get().LoadAsset<T>(path); 
    }
};

template<class T>
struct OD_API AssetVectorRef{
    std::vector<Ref<T>>& assets;

    AssetVectorRef(std::vector<Ref<T>>&  inAssets):assets(inAssets){}

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