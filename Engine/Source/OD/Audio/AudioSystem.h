#pragma once
#include "OD/Defines.h"
#include "OD/Serialization/Serialization.h"
#include "OD/Scene/Scene.h"
#include "OD/Core/AssetManager.h"
#include "AudioClip.h"
#include "AudioDef.h"
//#include <soloud.h>

namespace OD{

enum class AudioSourceMode{ Mode2D, Mode3D };    
enum Audio3dAttenuation{
    // No attenuation
    NoAttenuation = 0,
    // Inverse distance attenuation model
    InverseDistance = 1,
    // Linear distance attenuation model
    LinearDistance = 2,
    // Exponential distance attenuation model
    ExponentialDistance = 3
};

struct OD_API AudioSourceComponent{
    friend class AudioSystem;

    Ref<AudioClip> clip = nullptr;
    float minDistance = 10.0f; //Inside this radius, the sound is full volume.
    float maxDistance = 60.0f; // Beyond this, sound fades toward silence but doesn’t hard cut.
    float attenuationRolloff = 1.0f;
    AudioSourceMode mode = AudioSourceMode::Mode3D; 
    Audio3dAttenuation attenuation = Audio3dAttenuation::InverseDistance;
    int maxOnShotPlay = -1;

    bool IsPlaying() const;
    void Play();
    void Stop();
    void PlayOneShot(Ref<AudioClip> clip);
    void SetPosition(const Vector3& pos);
    void SetVolume(float vol);
    void SetPitch(float p);
    void SetLoop(bool l);

    static void OnGui(Entity& e, Scene& scene);

    template <class Archive>
    void serialize(Archive& ar){
        AssetRefSerialize<AudioClip> assetRef(clip);
        ArchiveDumpNVP(ar, assetRef);

        ArchiveDumpNVP(ar, minDistance);
        ArchiveDumpNVP(ar, maxDistance);
        ArchiveDumpNVP(ar, attenuationRolloff);
        ArchiveDumpNVP(ar, mode);
        ArchiveDumpNVP(ar, attenuation);

        ArchiveDumpNVP(ar, maxOnShotPlay);

        ArchiveDumpNVP(ar, volume);
        ArchiveDumpNVP(ar, pitch);
        ArchiveDumpNVP(ar, loop);
    }

private:
    Vector3 position = Vector3(0.0f);// 3D Position
    //SoLoud::Soloud* soloud = nullptr;
    //SoLoud::handle handle = 0;
    AUDIO_COMP_DATA
    //std::vector<SoLoud::handle> oneShots;
    float volume = 1.0f;
    float pitch = 1.0f;
    bool toPlay = false;
    bool loop = false;
};

struct OD_API AudioSettings{
    int maxActiveVoiceCount = 128;
    float volume = 1;

    template<class Archive>
    void serialize(Archive& ar){
        ArchiveDumpNVP(ar, maxActiveVoiceCount);
        ArchiveDumpNVP(ar, volume);
    }

    void OnImGuiRender();
};

class OD_API AudioSystem: public System{
public:
    AudioSystem();
    ~AudioSystem() override;

    virtual void OnStop(Scene& scene) override;
    virtual int Type() override { return SystemType::Stand; }
    virtual void Update(Scene& scene) override;

    AudioSettings& GetSettings();
    void UpdateSettings();

private:
    //AudioSettings settings;
    float m_3dUpdateTimer = 0.0f;
};

void AudioModuleInit();

}