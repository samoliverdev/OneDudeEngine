#pragma once
#include "OD/Defines.h"
#include "OD/Base.h"
#include "OD/Serialization/Serialization.h"
#include <string>
#include <filesystem>

namespace OD{

struct OD_API Project{
    std::string name = "Untitled";
    std::string startScene;
    std::string assetDirectory = "";
    std::string scriptModulePath = "";

    std::string projectDirectory = "";
    
    template<class Archive>
    void serialize(Archive& ar){
        ArchiveDumpNVP(ar, name);
        ArchiveDumpNVP(ar, startScene);
        ArchiveDumpNVP(ar, assetDirectory);
        ArchiveDumpNVP(ar, scriptModulePath);
    }
};

class OD_API ProjectManager{
public:
    static Ref<Project> GetActiveProject();
    //static const Project& GetActiveProject();
    static Ref<Project> NewProject(const char* path, const char* name, const char* templatePath = nullptr);
    static Ref<Project> LoadProject(const char* path);
};

}