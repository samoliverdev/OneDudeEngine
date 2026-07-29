#pragma once
#include "OD/Base.h"
#include "OD/Serialization/Serialization.h"
#include "Package.h"
#include <cstdint>

namespace OD{

#define USE_EXPERIMENTAL_ALLOCATOR
#define USE_WEAK_PTR

constexpr uint32_t INVALID_RESOURCE_ID = UINT32_MAX;

//TODO: Maybe rename to Resource
class OD_API Resource{
    template<typename T> friend class ResourceAllocator;
public:
    enum class SaveType{ 
        SettingOnly, 
        AssetBinary, //Portable binary to use for project data 
        FinalBinary //Final binary to use for final assets packing data
    };

    virtual ~Resource() = default; //virtual ~Asset(){}
    virtual std::string& Path();
    bool PathIsValid();
    virtual void OnGui();
    virtual void Reload();
    virtual bool Save(const std::string& outPath, SaveType type);
    virtual bool LoadFromFile(const std::string& path);
    virtual bool LoadFromPackage(const std::string& path, Package& package);
    virtual std::vector<std::string> GetFileAssociations();
    bool HasFileExtension(const std::string& fileExtension);
    //Ref<Package> GetPackage();

    virtual size_t RamUsage(){ return 0; }
    virtual size_t VRamUsage(){ return 0; }

    template<typename T, typename... Args>
    static Ref<T> CreateFromFile(const std::string& path, Args&&... args){
        static_assert(std::is_base_of_v<Resource, T>, "T must derive from Asset");
        Ref<T> asset = CreateRef<T>(std::forward<Args>(args)...);
        if(!asset->LoadFromFile(path)) return nullptr;
        return asset;
    }

    template<typename T, typename... Args>
    static Ref<T> CreateFromPackage(const std::string& path, Package& package, Args&&... args){
        static_assert(std::is_base_of_v<Resource, T>, "T must derive from Asset");
        Ref<T> asset = CreateRef<T>(std::forward<Args>(args)...);
        if(!asset->LoadFromPackage(path)) return nullptr;
        return asset;
    }

    inline uint32_t GetId(){ return resourceId; }

protected:
    std::string path = "#Memory";
    //Ref<Package> package = nullptr;
private: 
    uint32_t resourceId = INVALID_RESOURCE_ID;
};

}
