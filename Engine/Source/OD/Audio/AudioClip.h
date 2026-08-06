#pragma once
#include "OD/Defines.h"
#include "OD/Core/Resource.h"
#include "AudioDef.h"

namespace OD{

enum class AudioClipLoadType {
    DecompressOnLoad,     // SFX
    Streaming             // music
};

class OD_API AudioClip: public Resource{
    friend struct AudioSourceComponent;
    friend class AudioSystem;
public:
    AudioClip() = default;
    AudioClip(const std::string& filePath);
    ~AudioClip();

    bool LoadFromFile(const std::string& path) override;
    bool LoadFromPackage(const std::string& path, Package& package) override;
    std::vector<std::string> GetFileAssociations() override;

    inline AudioClipLoadType LoadType(){ return loadType; }

private:
    AudioClipLoadType loadType = AudioClipLoadType::Streaming;// AudioClipLoadType::DecompressOnLoad;
    AUDIO_CLIP_DATA
    bool loaded = false;
};

}