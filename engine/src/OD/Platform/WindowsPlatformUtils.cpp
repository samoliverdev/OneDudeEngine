#include "OD/Utils/PlatformUtils.h"
#include "tinyfiledialogs/tinyfiledialogs.h"

namespace OD{

std::string FileDialogs::OpenFile(const char* filter){
    const char* r = tinyfd_openFileDialog("", NULL, 0, NULL, NULL, 0);

    if(r != NULL) return std::string(r);
    return std::string();
}

std::string FileDialogs::SaveFile(const char* filter){
    const char* r = tinyfd_saveFileDialog("", NULL, 0, NULL, NULL);

    if(r != NULL) return std::string(r);
    return std::string();
}

}