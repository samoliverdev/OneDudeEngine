#pragma once
#include "OD/Defines.h"
#include "OD/Serialization/Serialization.h"
#include "OD/Scene/Scene.h"
#include "AudioClip.h"
#include <soloud.h>

namespace OD{

struct OD_API AudioSourceComponent{
    friend class AudioSystem;

    Ref<AudioClip> clip = nullptr;

    void Play();
    void Stop();

    static void OnGui(Entity& e);

    template <class Archive>
    void serialize(Archive& ar){
        AssetRefSerialize<AudioClip> assetRef(clip);
        ArchiveDumpNVP(ar, assetRef);
    }

private:
    SoLoud::Soloud* soloud = nullptr;
    SoLoud::handle handle = 0;
    bool toPlay = false;
};

class OD_API AudioSystem: public System{
public:
    AudioSystem(Scene* scene);
    ~AudioSystem() override;

    AudioSystem* Clone(Scene* inScene) const override{ 
        AudioSystem* system = new AudioSystem(inScene);
        return system; 
    }
    
    virtual SystemType Type() override { return SystemType::Stand; }
    virtual void Update() override;
};

void AudioModuleInit();

}