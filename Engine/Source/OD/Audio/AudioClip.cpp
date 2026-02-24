#include "OD/pch.h"
#include "AudioClip.h"

namespace OD{

AudioClip::AudioClip(const std::string& filePath){
    LoadFromFile(filePath);
}

bool AudioClip::LoadFromFile(const std::string& path){
    auto result = sample.load(path.c_str());
    if(result != SoLoud::SO_NO_ERROR){
        LogError("Erro to load: {}", path);
        return false;
    }

    return result == SoLoud::SO_NO_ERROR;
}

bool AudioClip::LoadFromPackage(const std::string& path, Package& package){
    void* data = nullptr;
    size_t size;
    if(package.ReadFileData(path.c_str(), data, size) == false){
        package.FreeFileData(data);
        return false;
    }

    auto result = sample.loadMem((unsigned char*)data, size, true);
    if(result != SoLoud::SO_NO_ERROR){
        LogError("Erro to load: {}", path);
        package.FreeFileData(data);
        return false;
    }

    package.FreeFileData(data);
    return result == SoLoud::SO_NO_ERROR;
}

std::vector<std::string> AudioClip::GetFileAssociations(){
    return std::vector<std::string>{
        ".mp3", ".wav"
    };
}

}