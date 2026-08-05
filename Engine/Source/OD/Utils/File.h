#pragma once
#include "OD/Defines.h"
#ifdef _WIN32
#include <Windows.h>
#endif

#include <filesystem>

namespace OD{


inline bool FileExists(const std::string& path){
    return std::filesystem::exists(path);
}

//Source: https://gist.github.com/Jacob-Tate/7b326a086cf3f9d46e32315841101109

//Returns the absolute path of the executable
inline std::filesystem::path GetAbsExePath(){
    #if defined(_MSC_VER)
        wchar_t path[FILENAME_MAX] = { 0 };
        GetModuleFileNameW(nullptr, path, FILENAME_MAX);
        return std::filesystem::path(path);
    #else
        char path[FILENAME_MAX];
        ssize_t count = readlink("/proc/self/exe", path, FILENAME_MAX);
        return std::filesystem::path(std::string(path, (count > 0) ? count: 0));
    #endif
}

inline std::filesystem::path GetAbsExeDirectory(){
    #if defined(_MSC_VER)
        wchar_t path[FILENAME_MAX] = { 0 };
        GetModuleFileNameW(nullptr, path, FILENAME_MAX);
        return std::filesystem::path(path).parent_path().string();
    #else
        char path[FILENAME_MAX];
        ssize_t count = readlink("/proc/self/exe", path, FILENAME_MAX);
        return std::filesystem::path(std::string(path, (count > 0) ? count: 0)).parent_path().string();
    #endif
}

inline std::string RemoveBasePath(const std::string& fullPath, const std::string& basePath) {
    // Ensure both paths use the same slash convention (Windows-style)
    std::string normalizedFullPath = fullPath;
    std::string normalizedBasePath = basePath;

    // Optionally normalize slashes (if needed)
    std::replace(normalizedFullPath.begin(), normalizedFullPath.end(), '\\', '/');
    std::replace(normalizedBasePath.begin(), normalizedBasePath.end(), '\\', '/');

    // Ensure basePath ends with a slash
    if (!normalizedBasePath.empty() && normalizedBasePath.back() != '/')
        normalizedBasePath += '/';

    // Check if basePath is a prefix
    if (normalizedFullPath.find(normalizedBasePath) == 0) {
        return normalizedFullPath.substr(normalizedBasePath.length());
    }

    // If basePath isn't found, return original fullPath
    return fullPath;
}

}