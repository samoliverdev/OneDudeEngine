#include "OD/pch.h"
#include "AudioSystem.h"
#include "AudioClip.h"
#include "OD/Scene/SceneManager.h"
#include "OD/Core/Instrumentor.h"
#include "OD/Core/ImGui.h"
#include <soloud.h>
#include <soloud_wav.h>
#include <soloud_speech.h>
#include <soloud_thread.h>

namespace OD{

void AudioModuleInit(){
    AssetTypesDB::Get().RegisterAssetType<AudioClip>(".mp3", [](const std::string& path){ return AssetManager::Get().LoadAsset<AudioClip>(path); });
    AssetTypesDB::Get().RegisterAssetType<AudioClip>(".wav", [](const std::string& path){ return AssetManager::Get().LoadAsset<AudioClip>(path); });

    SceneManager::Get().RegisterCoreComponent<AudioSourceComponent>("AudioSourceComponent", "Audio");
    SceneManager::Get().RegisterSystem<AudioSystem>("AudioSystem");
}

SoLoud::Soloud soloud;
bool hasInited = false;

void AudioSourceComponent::Play(){
    if(!clip){
        LogWarning("AudioSourceComponent::Play - No clip assigned.");
        return;
    }

    if(!soloud){
        toPlay = true;
        return;
    }

    Stop();

    if(mode == AudioSourceMode::Mode3D){
        //clip->sample.set3dMinMaxDistance(minDistance, maxDistance);
        //clip->sample.set3dAttenuation(SoLoud::AudioSource::LINEAR_DISTANCE, attenuationRolloff);
        //clip->sample.setInaudibleBehavior(true, true);
        handle = soloud->play3d(
            clip->sample, 
            position.x, position.y, position.z,
            0, 0, 0,
            volume, 
            true
        );
    } else {
        handle = soloud->play(clip->sample); // Som 2D sem posição
    }
    
    soloud->setLooping(handle, loop);
    soloud->setVolume(handle, volume);
    soloud->setRelativePlaySpeed(handle, pitch);
    //soloud->set3dSourcePosition(handle, position.x, position.y, position.z);
    
    if(mode == AudioSourceMode::Mode3D){
        //soloud->set3dSourcePosition(handle, position.x, position.y, position.z);
        soloud->set3dSourceMinMaxDistance(handle, minDistance, maxDistance);
        soloud->set3dSourceAttenuation(handle, attenuation, attenuationRolloff);
        //soloud->setInaudibleBehavior(handle, true, true);
        //soloud->update3dAudio();
        soloud->setPause(handle, false);
    } else {
        // 2D sound: não usa configurações 3D, ou pode resetar se quiser
    }
}

void AudioSourceComponent::Stop(){
    if(soloud != nullptr){
        soloud->stop(handle);
    }
}

void AudioSourceComponent::PlayOneShot(Ref<AudioClip> clip){
    //TOOD: save clip too, to avoid unload 

    if(!soloud) return;

    if(maxOnShotPlay > 0 &&  oneShots.size() >= maxOnShotPlay) return;

    SoLoud::handle h; 
    if(mode == AudioSourceMode::Mode3D){
        //clip->sample.set3dMinMaxDistance(minDistance, maxDistance);
        //clip->sample.set3dAttenuation(SoLoud::AudioSource::LINEAR_DISTANCE, attenuationRolloff);
        //clip->sample.setInaudibleBehavior(true, true);
        h = soloud->play3d(
            clip->sample, 
            position.x, position.y, position.z,
            0, 0, 0,
            volume, 
            true
        );
    } else {
        h = soloud->play(clip->sample); // Som 2D sem posição
    }

    Assert(h != 0);

    soloud->setLooping(h, false);
    soloud->setVolume(h, volume);
    soloud->setRelativePlaySpeed(h, pitch);
    
    if(mode == AudioSourceMode::Mode3D){
        soloud->set3dSourceMinMaxDistance(h, minDistance, maxDistance);
        soloud->set3dSourceAttenuation(h, attenuation, attenuationRolloff);
        //soloud->update3dAudio();
        soloud->setInaudibleBehavior(h, false, true);
        soloud->setPause(h, false);
    } else {
        // 2D sound: não usa configurações 3D, ou pode resetar se quiser
    }
    
    oneShots.push_back(h);
}

void AudioSourceComponent::SetPosition(const Vector3& pos){
    position = pos;
    if (soloud && mode == AudioSourceMode::Mode3D){
        soloud->set3dSourcePosition(handle, pos.x, pos.y, pos.z);
    }
}

void AudioSourceComponent::SetVolume(float vol){
    volume = vol;
    if(soloud){
        soloud->setVolume(handle, vol);
    }
}

void AudioSourceComponent::SetPitch(float p){
    pitch = p;
    if(soloud){
        soloud->setRelativePlaySpeed(handle, p);
    }
}

void AudioSourceComponent::SetLoop(bool l){
    loop = l;
    if(soloud){
        soloud->setLooping(handle, l);
    }
}

void AudioSourceComponent::OnGui(Entity& e, Scene& scene){
    AudioSourceComponent& audioSource = scene.GetComponent<AudioSourceComponent>(e);

    if(ImGui::Checkbox("Loop", &audioSource.loop)){
        audioSource.SetLoop(audioSource.loop);
    }
    if(ImGui::DragFloat("Volume", &audioSource.volume)){
        audioSource.SetVolume(audioSource.volume);
    }
    if(ImGui::DragFloat("Pitch", &audioSource.pitch)){
        audioSource.SetPitch(audioSource.pitch);
    }

    ImGui::DragFloat("minDistance", &audioSource.minDistance);
    ImGui::DragFloat("maxDistance", &audioSource.maxDistance);
    ImGui::DragFloat("attenuationRolloff", &audioSource.attenuationRolloff);
    ImGui::DrawEnumCombo<AudioSourceMode>("mode", &audioSource.mode);
    ImGui::DrawEnumCombo<Audio3dAttenuation>("attenuation", &audioSource.attenuation);
}

AudioSystem::AudioSystem(){
    name = "AudioSystem";
    //soloud.init(); 
    //Erro: call init twice on playing mode
}

AudioSystem::~AudioSystem(){
    //LogWarningExtra("AudioSystem::~AudioSystem");
    if(hasInited == true){
        soloud.deinit();
        hasInited = false;
    }
}

void AudioSystem::Update(Scene& scene){
    OD_PROFILE_SCOPE("ScriptSystem::Update");

    /*if(hasInited == false){
        soloud.init();
        hasInited = true;
    }

    auto audioView = scene->GetRegistry().view<AudioSourceComponent>();
    for(auto e: audioView){
        AudioSourceComponent& audioSource = audioView.get<AudioSourceComponent>(e);
        if(audioSource.soloud == nullptr) audioSource.soloud = &soloud;
        if(audioSource.toPlay == true){
            audioSource.toPlay = false;
            audioSource.Play();
        }
    }*/

    if(!hasInited){
        soloud.init();
        //soloud.setMaxActiveVoiceCount(128);
        hasInited = true;
    }

    auto cam = scene.GetMainCamera();
    Assert(cam != EntityNull);
    TransformComponent& camTrans = scene.GetComponent<TransformComponent>(cam);
    Vector3 pos = camTrans.Position();
    Vector3 forward = camTrans.Back();
    Vector3 up = camTrans.Up();
    
    soloud.set3dListenerPosition(pos.x, pos.y, pos.z);  // Simple listener default
    soloud.set3dListenerAt(forward.x, forward.y, forward.z);
    soloud.set3dListenerUp(up.x, up.y, up.z);
    soloud.set3dListenerVelocity(0, 0, 0);  // Optional for Doppler

    auto audioView = scene.GetRegistry().view<AudioSourceComponent, TransformComponent>();
    for(auto e : audioView){
        AudioSourceComponent& audio = audioView.get<AudioSourceComponent>(e);
        if(audio.soloud == nullptr) audio.soloud = &soloud;

        if(audio.toPlay){
            audio.toPlay = false;
            audio.Play();
        }

        // Optional: Update position every frame
        if(audio.mode == AudioSourceMode::Mode3D){
            TransformComponent& trans = audioView.get<TransformComponent>(e);
            auto pos = trans.Position();
            audio.position = pos;
            soloud.set3dSourcePosition(audio.handle, pos.x, pos.y, pos.z);
        }
        //soloud.set3dSourcePosition(audio.handle, audio.position.x, audio.position.y, audio.position.z);

        for(auto it = audio.oneShots.begin(); it != audio.oneShots.end(); ){
            if(!soloud.isValidVoiceHandle(*it)){
                it = audio.oneShots.erase(it);
            } else {
                ++it;
            }
        }
        for(auto h : audio.oneShots){
            soloud.set3dSourcePosition(h, pos.x, pos.y, pos.z);
        }
    }

    soloud.update3dAudio();

    LogInfo("Active voices: {}", soloud.getActiveVoiceCount());
}

}
