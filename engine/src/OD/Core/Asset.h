#pragma once

#include "OD/Defines.h"
#include "OD/Serialization/Serialization.h"

namespace OD{

class AssetManager;

class OD_API Asset{
public:
    virtual ~Asset(){}

    inline std::string& Path(){ return path; }
    inline void Path(const std::string& inPath){ path = inPath; }

    virtual void OnGui(){}
    virtual void Reload(){}
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

        //asset = CreateRef<T>();
        //asset->LoadFromFile(path);

        asset = AssetManager::Get().LoadAsset<T>(path); 
    }
};

}