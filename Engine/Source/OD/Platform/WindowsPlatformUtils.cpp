#include "OD/pch.h"
#include "Platform.h"
#include "OD/Base.h"
#include "tinyfiledialogs/tinyfiledialogs.h"
#include <filesystem>

namespace OD{

//Fixme:: r memory leak
std::string Platform::OpenFolder(){
    const char* r = tinyfd_selectFolderDialog("Open Project", NULL);
    if(r != NULL) return std::string(r);
    
    return std::string();
}

//Fixme:: r memory leak
std::string Platform::OpenFile(const char* filter){
    std::string curPath = std::filesystem::current_path().string() + "/";
    LogWarning("CurPath: %s", curPath.c_str());

    if(strcmp(filter, "") == 0){
        const char* r = tinyfd_openFileDialog("", curPath.c_str(), 0, NULL, NULL, 0);
        if(r != NULL) return std::string(r);
    } else {
        const char* r = tinyfd_openFileDialog("", curPath.c_str(), 1, &filter, NULL, 0);
        if(r != NULL) return std::string(r);
    }
    
    return std::string();
}

//Fixme:: r memory leak
std::string Platform::SaveFile(const char* filter){
    if(strcmp(filter, "") == 0){
        const char* r = tinyfd_saveFileDialog("", NULL, 0, NULL, NULL);
        if(r != NULL) return std::string(r);
    } else {
        const char* r = tinyfd_saveFileDialog("", NULL, 1, &filter, NULL);
        if(r != NULL) return std::string(r);
    }
    
    return std::string();
}


}