#include "AudioSystem.h"
#include "AudioClip.h"
#include <soloud.h>
#include <soloud_wav.h>
#include <soloud_speech.h>
#include <soloud_thread.h>
#include "OD/Scene/SceneManager.h"

namespace OD{

void AudioModuleInit(){
    AssetTypesDB::Get().RegisterAssetType<AudioClip>(".mp3", [](const std::string& path){ return AssetManager::Get().LoadAsset<AudioClip>(path); });
    AssetTypesDB::Get().RegisterAssetType<AudioClip>(".wav", [](const std::string& path){ return AssetManager::Get().LoadAsset<AudioClip>(path); });

    SceneManager::Get().RegisterCoreComponent<AudioSourceComponent>("AudioSourceComponent");
    SceneManager::Get().RegisterSystem<AudioSystem>("AudioSystem");
}

SoLoud::Soloud soloud;
bool hasInited = false;

void AudioSourceComponent::Play(){
    LogWarning("AudioSourceComponent::Play");

    if(soloud == nullptr){
        toPlay = true;
    } else {
        handle = soloud->play(clip->sample);
    }
}

void AudioSourceComponent::Stop(){
    if(soloud != nullptr){
        soloud->stop(handle);
    }
}

void AudioSourceComponent::OnGui(Entity& e){}

AudioSystem::AudioSystem(Scene* inScene):System(inScene){
    //soloud.init(); 
    //Erro: call init twice on playing mode
}

AudioSystem::~AudioSystem(){
    LogWarningExtra("AudioSystem::~AudioSystem");
    if(hasInited == true){
        soloud.deinit();
        hasInited = false;
    }
}

void AudioSystem::Update(){
    if(hasInited == false){
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
    }
}

}
