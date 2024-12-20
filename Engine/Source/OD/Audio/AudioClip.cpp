#include "AudioClip.h"

namespace OD{

AudioClip::AudioClip(const std::string& filePath){
    LoadFromFile(filePath);
}

bool AudioClip::LoadFromFile(const std::string& path){
    auto result = sample.load(path.c_str());
    if(result != SoLoud::SO_NO_ERROR){
        LogError("Erro to load: %s", path.c_str());
        return false;
    }

    return result == SoLoud::SO_NO_ERROR;
}

std::vector<std::string> AudioClip::GetFileAssociations(){
    return std::vector<std::string>{
        ".mp3", ".wav"
    };
}

}