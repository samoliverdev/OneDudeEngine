#include "AudioSystem.h"
#include "AudioClip.h"
#include <soloud.h>
#include <soloud_wav.h>
#include <soloud_speech.h>
#include <soloud_thread.h>
#include "OD/Scene/SceneManager.h"
#include "OD/Core/Instrumentor.h"

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
    if(!clip){
        LogWarning("AudioSourceComponent::Play - No clip assigned.");
        return;
    }

    if(!soloud){
        toPlay = true;
        return;
    }

    //Stop();

    if(mode == AudioSourceMode::Mode3D){
        handle = soloud->play3d(clip->sample, position.x, position.y, position.z);
    } else {
        handle = soloud->play(clip->sample); // Som 2D sem posição
    }
    
    Apply3DSettings();
}

void AudioSourceComponent::Stop(){
    if(soloud != nullptr){
        soloud->stop(handle);
    }
}

void AudioSourceComponent::SetPosition(const Vector3& pos) {
    position = pos;
    if (soloud && mode == AudioSourceMode::Mode3D){
        soloud->set3dSourcePosition(handle, pos.x, pos.y, pos.z);
    }
}

void AudioSourceComponent::SetVolume(float vol) {
    volume = vol;
    if (soloud) {
        soloud->setVolume(handle, vol);
    }
}

void AudioSourceComponent::SetPitch(float p) {
    pitch = p;
    if (soloud) {
        soloud->setRelativePlaySpeed(handle, p);
    }
}

void AudioSourceComponent::SetLoop(bool l) {
    loop = l;
    if (soloud) {
        soloud->setLooping(handle, l);
    }
}

void AudioSourceComponent::Apply3DSettings() {
    soloud->setLooping(handle, loop);
    soloud->setVolume(handle, volume);
    soloud->setRelativePlaySpeed(handle, pitch);
    //soloud->set3dSourcePosition(handle, position.x, position.y, position.z);
    
    if(mode == AudioSourceMode::Mode3D){
        //soloud->set3dSourcePosition(handle, position.x, position.y, position.z);
        soloud->set3dSourceMinMaxDistance(handle, minDistance, maxDistance);
        soloud->set3dSourceAttenuation(handle, SoLoud::AudioSource::INVERSE_DISTANCE, attenuationRolloff);
    } else {
        // 2D sound: não usa configurações 3D, ou pode resetar se quiser
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
}

AudioSystem::AudioSystem(Scene* inScene):System(inScene){
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

void AudioSystem::Update(){
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
        hasInited = true;
    }

    auto cam = scene->GetMainCamera();
    Assert(cam != EntityNull);
    TransformComponent& camTrans = scene->GetComponent<TransformComponent>(cam);
    Vector3 pos = camTrans.Position();
    Vector3 forward = camTrans.Back();
    Vector3 up = camTrans.Up();
    
    soloud.set3dListenerPosition(pos.x, pos.y, pos.z);  // Simple listener default
    soloud.set3dListenerAt(forward.x, forward.y, forward.z);
    soloud.set3dListenerUp(up.x, up.y, up.z);
    soloud.set3dListenerVelocity(0, 0, 0);  // Optional for Doppler

    auto audioView = scene->GetRegistry().view<AudioSourceComponent, TransformComponent>();
    for(auto e : audioView){
        AudioSourceComponent& audio = audioView.get<AudioSourceComponent>(e);
        if(audio.soloud == nullptr) audio.soloud = &soloud;

        if(audio.toPlay){
            audio.toPlay = false;
            audio.Play();
        }

        // Optional: Update position every frame
        TransformComponent& trans = audioView.get<TransformComponent>(e);
        auto pos = trans.Position();
        soloud.set3dSourcePosition(audio.handle, pos.x, pos.y, pos.z);
        //soloud.set3dSourcePosition(audio.handle, audio.position.x, audio.position.y, audio.position.z);
    }

    soloud.update3dAudio();
}

}
