#include "OD/Utils/PlatformUtils.h"
#include "tinyfiledialogs/tinyfiledialogs.h"
#include <string.h>
#include <Windows.h>

namespace OD{

std::string FileDialogs::OpenFolder(){
    const char* r = tinyfd_selectFolderDialog("Open Project", NULL);
    if(r != NULL) return std::string(r);
    
    return std::string();
}

std::string FileDialogs::OpenFile(const char* filter){
    if(strcmp(filter, "") == 0){
        const char* r = tinyfd_openFileDialog("", NULL, 0, NULL, NULL, 0);
        if(r != NULL) return std::string(r);
    } else {
        const char* r = tinyfd_openFileDialog("", NULL, 1, &filter, NULL, 0);
        if(r != NULL) return std::string(r);
    }
    
    return std::string();
}

std::string FileDialogs::SaveFile(const char* filter){
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