#pragma once
#include "OD/Base.h"
#include "OD/Serialization/SerializationCore.h"
#include <string>
#include <filesystem>

namespace OD{

struct OD_API Project{
    std::string name = "Untitled";
    std::string startScene = "";
    std::string assetDirectory = "";
    std::string scriptModulePath = "";
    std::string defaultPackagePath = "";

    std::string projectDirectory = "";
    
    template<class Archive>
    void serialize(Archive& ar){
        ArchiveDumpNVP(ar, name);
        ArchiveDumpNVP(ar, startScene);
        ArchiveDumpNVP(ar, assetDirectory);
        ArchiveDumpNVP(ar, scriptModulePath);
        ArchiveDumpNVP(ar, defaultPackagePath);
    }
};

class OD_API ProjectManager{
public:
    static Ref<Project> GetActiveProject();
    static Ref<Project> NewProject(const char* path, const char* name, const char* templatePath = nullptr);
    static Ref<Project> LoadProject(const char* path);
    static Ref<Project> LoadProject(const char* path, Ref<Project> proj);
};

}