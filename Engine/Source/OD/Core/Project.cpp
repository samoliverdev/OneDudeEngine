#include "Project.h"
#include "OD/Serialization/SerializationFull.h"
#include <fstream>

namespace OD{

Ref<Project> activeProject = nullptr;

/*const Project& ProjectManager::GetActiveProject(){ 
    Assert(activeProject != nullptr);
    return *activeProject; 
}*/

Ref<Project> ProjectManager::GetActiveProject(){ 
    Assert(activeProject != nullptr); 
    return activeProject; 
}

Ref<Project> ProjectManager::NewProject(const char* path, const char* name, const char* templatePath){
    return nullptr;
}

Ref<Project> ProjectManager::LoadProject(const char* path){
    activeProject = CreateRef<Project>();

    //std::filesystem::current_path(projectPath);
    LogInfo("Loading Project Path: %s", path);
    LogWarning("Cur Path: %s", std::filesystem::current_path().string().c_str());

    #if !defined(__EMSCRIPTEN__)
    
    std::string _projectPath(path);
    std::string projectSettingsPath = _projectPath + "ProjectSettings.proj";

    std::ifstream stream(projectSettingsPath);
    if(stream.fail()){
        std::ofstream os(projectSettingsPath);
        cereal::JSONOutputArchive ar(os);
        ArchiveDumpNamed(ar, "ProjectSettings", *activeProject);
    } else {
        cereal::JSONInputArchive ar{stream};
        ArchiveDumpNamed(ar, "ProjectSettings", *activeProject);
    }

    std::filesystem::current_path(_projectPath + "Content");

    LogWarning("Cur Path: %s", std::filesystem::current_path().string().c_str());

    #endif

    return activeProject;
}

}