#pragma once
#include <string>

namespace OD{

inline std::string GetFileExtension(const std::string& path){
    size_t dotPos = path.rfind('.');
    return (dotPos != std::string::npos) ? path.substr(dotPos + 1) : "";
};

}