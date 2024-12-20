#pragma once
#include "OD/Defines.h"
#include "OD/Core/Asset.h"
#include <soloud_wav.h>

namespace OD{

class OD_API AudioClip: public Asset{
    friend struct AudioSourceComponent;
    friend class AudioSystem;
public:
    AudioClip() = default;
    AudioClip(const std::string& filePath);

    bool LoadFromFile(const std::string& path) override;
    std::vector<std::string> GetFileAssociations() override;

private:
    SoLoud::Wav sample;
};

}