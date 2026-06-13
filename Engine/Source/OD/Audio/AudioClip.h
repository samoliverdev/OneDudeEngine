#pragma once
#include "OD/Defines.h"
#include "OD/Core/Asset.h"
#include "AudioDef.h"

namespace OD{

class OD_API AudioClip: public Asset{
    friend struct AudioSourceComponent;
    friend class AudioSystem;
public:
    AudioClip() = default;
    AudioClip(const std::string& filePath);
    ~AudioClip();

    bool LoadFromFile(const std::string& path) override;
    bool LoadFromPackage(const std::string& path, Package& package) override;
    std::vector<std::string> GetFileAssociations() override;

private:
    AUDIO_CLIP_DATA
    bool loaded = false;
};

}