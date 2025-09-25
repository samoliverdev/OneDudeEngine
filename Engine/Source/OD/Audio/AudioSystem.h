#pragma once
#include "OD/Defines.h"
#include "OD/Serialization/Serialization.h"
#include "OD/Scene/Scene.h"
#include "AudioClip.h"
#include <soloud.h>

namespace OD{

enum class AudioSourceMode{ Mode2D, Mode3D };    

struct OD_API AudioSourceComponent{
    friend class AudioSystem;

    Ref<AudioClip> clip = nullptr;
    float minDistance = 10.0f; //Inside this radius, the sound is full volume.
    float maxDistance = 60.0f; // Beyond this, sound fades toward silence but doesn’t hard cut.
    float attenuationRolloff = 1.0f;
    AudioSourceMode mode = AudioSourceMode::Mode3D; 
    
    void Play();
    void Stop();
    void SetPosition(const Vector3& pos);
    void SetVolume(float vol);
    void SetPitch(float p);
    void SetLoop(bool l);
    void Apply3DSettings();

    static void OnGui(Entity& e, Scene& scene);

    template <class Archive>
    void serialize(Archive& ar){
        AssetRefSerialize<AudioClip> assetRef(clip);
        ArchiveDumpNVP(ar, assetRef);
    }

private:
    bool loop = false;
    float volume = 1.0f;
    float pitch = 1.0f;
    Vector3 position = Vector3(0.0f);// 3D Position

    SoLoud::Soloud* soloud = nullptr;
    SoLoud::handle handle = 0;
    bool toPlay = false;
};

class OD_API AudioSystem: public System{
public:
    AudioSystem();
    ~AudioSystem() override;

    virtual int Type() override { return SystemType::Stand; }
    virtual void Update(Scene& scene) override;
};

void AudioModuleInit();

}