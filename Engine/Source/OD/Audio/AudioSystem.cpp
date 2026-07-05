#include "OD/pch.h"
#include "AudioSystem.h"
#include "AudioClip.h"
#include "OD/Scene/SceneManager.h"
#include "OD/Core/Instrumentor.h"
#include "OD/Core/ImGui.h"
#include "OD/Core/GlobalSettings.h"
#include "OD/Core/Time.h"

#ifdef AUDIO_BACKEND_SOLOUD
#include <soloud.h>
#include <soloud_speech.h>
#include <soloud_thread.h>
#include <thread>

namespace OD{

std::thread::id mainThreadID;

inline bool IsMainThread(){
    return std::this_thread::get_id() == mainThreadID;
}

void AudioModuleInit(){
    AssetTypesDB::Get().RegisterAssetType<AudioClip>(".mp3", [](const std::string& path){ return AssetManager::Get().LoadAsset<AudioClip>(path); });
    AssetTypesDB::Get().RegisterAssetType<AudioClip>(".wav", [](const std::string& path){ return AssetManager::Get().LoadAsset<AudioClip>(path); });

    SceneManager::Get().RegisterCoreComponent<AudioSourceComponent>("AudioSourceComponent", "Audio");
    SceneManager::Get().RegisterSystem<AudioSystem>("AudioSystem");

    OD::GlobalSettings::Get().Register<AudioSettings>("Audio");

    mainThreadID = std::this_thread::get_id();
}

SoLoud::Soloud globalSoloud;
bool hasInited = false;

bool AudioSourceComponent::IsPlaying() const{
    Assert(IsMainThread() && "update3dAudio must be called from main thread!");

    if(!soloud) return false;
    if(handle == 0) return false;
    return soloud->isValidVoiceHandle(handle);
}

void AudioSourceComponent::Play(){
    Assert(IsMainThread() && "update3dAudio must be called from main thread!");

    if(!clip){
        LogWarning("AudioSourceComponent::Play - No clip assigned.");
        return;
    }

    if(!soloud){
        toPlay = true;
        return;
    }

    if(IsPlaying()){
        return;
    }

    Stop();

    if(mode == AudioSourceMode::Mode3D){
        //INFO: Set this by clip fix some strange distance volume bug, Review this later if need
        clip->sample.set3dMinMaxDistance(minDistance, maxDistance);
        clip->sample.set3dAttenuation(attenuation, attenuationRolloff);
        //clip->sample.setInaudibleBehavior(true, true);
        handle = soloud->play3d(
            clip->sample, 
            position.x, position.y, position.z,
            0, 0, 0,
            volume, 
            false //true
        );
        LogInfo("Handle: {}", handle);
    } else {
        handle = soloud->play(clip->sample); // Som 2D sem posição
        LogInfo("Handle: {}", handle);
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
        //soloud->setPause(handle, false);
    } else {
        // 2D sound: não usa configurações 3D, ou pode resetar se quiser
    }
}

void AudioSourceComponent::Stop(){
    Assert(IsMainThread() && "update3dAudio must be called from main thread!");

    if(soloud != nullptr){
        soloud->stop(handle);
        handle = 0;
    }
}

void AudioSourceComponent::PlayOneShot(Ref<AudioClip> clip){
    return;
    //TOOD: save clip too, to avoid unload 
    Assert(IsMainThread() && "update3dAudio must be called from main thread!");

    if(!soloud) return;

    //if(maxOnShotPlay > 0 && oneShots.size() >= maxOnShotPlay) return;

    //attenuation = Audio3dAttenuation::LinearDistance;

    SoLoud::handle h; 
    if(mode == AudioSourceMode::Mode3D){
        //INFO: Set this by clip fix some strange distance volume bug, Review this later if need
        clip->sample.set3dMinMaxDistance(minDistance, maxDistance);
        clip->sample.set3dAttenuation(attenuation, attenuationRolloff);
        //clip->sample.setInaudibleBehavior(true, true);
        h = soloud->play3d(
            clip->sample, 
            position.x, position.y, position.z,
            0, 0, 0,
            volume, 
            false //true
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
        //soloud->setInaudibleBehavior(h, true, true);
        //soloud->update3dAudio();  
        //soloud->setPause(h, false);
    } else {
        // 2D sound: não usa configurações 3D, ou pode resetar se quiser
    }
    
    //oneShots.push_back(h);
}

void AudioSourceComponent::SetPosition(const Vector3& pos){
    Assert(IsMainThread() && "update3dAudio must be called from main thread!");
    //Assert(handle != 0);

    position = pos;
    if(soloud && mode == AudioSourceMode::Mode3D && handle != 0){
        soloud->set3dSourcePosition(handle, pos.x, pos.y, pos.z);
    }
}

void AudioSourceComponent::SetVolume(float vol){
    Assert(IsMainThread() && "update3dAudio must be called from main thread!");
    //Assert(handle != 0);

    volume = vol;
    if(soloud && handle != 0){
        soloud->setVolume(handle, vol);
    }
}

void AudioSourceComponent::SetPitch(float p){
    Assert(IsMainThread() && "update3dAudio must be called from main thread!");
    //Assert(handle != 0);

    pitch = p;
    if(soloud && handle != 0){
        soloud->setRelativePlaySpeed(handle, p);
    }
}

void AudioSourceComponent::SetLoop(bool l){
    Assert(IsMainThread() && "update3dAudio must be called from main thread!");
    //Assert(handle != 0);

    loop = l;
    if(soloud && handle != 0){
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

void AudioSettings::OnImGuiRender(){
    ImGui::DragInt("maxActiveVoiceCount", &maxActiveVoiceCount, 0, 255);
    ImGui::DragFloat("volume", &volume, 1, 0, 2);
}

AudioSystem::AudioSystem(){
    Assert(IsMainThread() && "update3dAudio must be called from main thread!");
    name = "AudioSystem";
    //soloud.init(); 
    //Erro: call init twice on playing mode
}

AudioSystem::~AudioSystem(){
    Assert(IsMainThread() && "update3dAudio must be called from main thread!");
    
    //LogWarningExtra("AudioSystem::~AudioSystem");
    if(hasInited == true){
        globalSoloud.deinit();
        hasInited = false;
    }
}

void AudioSystem::OnStop(Scene& scene){

}

AudioSettings& AudioSystem::GetSettings(){
    return GlobalSettings::Get().Get<AudioSettings>();
}

void AudioSystem::UpdateSettings(){
    if(hasInited == false) return;
    Assert(IsMainThread() && "update3dAudio must be called from main thread!");

    const auto& settings = GlobalSettings::Get().Get<AudioSettings>();
    globalSoloud.setMaxActiveVoiceCount(settings.maxActiveVoiceCount);
    globalSoloud.setGlobalVolume(settings.volume);
}

int m_3dFrameSkip = 0;

void AudioSystem::Update(Scene& scene){
    OD_PROFILE_SCOPE("ScriptSystem::Update");
    Assert(IsMainThread() && "update3dAudio must be called from main thread!");

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
        //auto r = globalSoloud.init();
        //auto r = globalSoloud.init(SoLoud::Soloud::CLIP_ROUNDOFF, SoLoud::Soloud::SDL2); // Windows only
        //auto r = globalSoloud.init(SoLoud::Soloud::CLIP_ROUNDOFF, SoLoud::Soloud::MINIAUDIO, 44100, 512, 2);  // sample rate, buffer, channels
        auto r = globalSoloud.init(SoLoud::Soloud::CLIP_ROUNDOFF, SoLoud::Soloud::WASAPI);
        Assert(r == 0);
        hasInited = true;
        UpdateSettings();
    }

    auto cam = scene.GetMainCamera();
    Assert(cam != EntityNull);
    TransformComponent& camTrans = scene.GetComponent<TransformComponent>(cam);
    Vector3 camPos = camTrans.Position();
    Vector3 forward = camTrans.Back();
    Vector3 up = camTrans.Up();
    
    globalSoloud.set3dListenerPosition(camPos.x, camPos.y, camPos.z);  // Simple listener default
    globalSoloud.set3dListenerAt(forward.x, forward.y, forward.z);
    globalSoloud.set3dListenerUp(up.x, up.y, up.z);
    globalSoloud.set3dListenerVelocity(0, 0, 0);  // Optional for Doppler

    auto audioView = scene.GetRegistry().view<AudioSourceComponent, TransformComponent>();
    for(auto e : audioView){
        AudioSourceComponent& audio = audioView.get<AudioSourceComponent>(e);
        if(audio.soloud == nullptr) audio.soloud = &globalSoloud;

        if(audio.toPlay){
            audio.toPlay = false;
            audio.Play();
        }

        //INFO: this "if(!soloud.isValidVoiceHandle(*it)){" is cause freeze on some mutex lock
        /*for(auto it = audio.oneShots.begin(); it != audio.oneShots.end(); ){
            if(!soloud.isValidVoiceHandle(*it)){
                it = audio.oneShots.erase(it);
            } else {
                ++it;
            }
        }*/

        // Optional: Update position every frame
        if(audio.mode == AudioSourceMode::Mode3D){
            TransformComponent& trans = audioView.get<TransformComponent>(e);
            auto pos = trans.Position();
            audio.position = pos;

            if(audio.handle == 0) continue;
            globalSoloud.set3dSourcePosition(audio.handle, pos.x, pos.y, pos.z);
            //Maybe no need update pos from one shot sounds
            /*for(auto h : audio.oneShots){
                soloud.set3dSourcePosition(h, pos.x, pos.y, pos.z);
            }*/
        }
        //soloud.set3dSourcePosition(audio.handle, audio.position.x, audio.position.y, audio.position.z); 
    }

    /*std::this_thread::yield(); 
    LogInfo("Active voices: {}", globalSoloud.getActiveVoiceCount());
    LogInfo("Inside mutex? (can't check directly, but watch for crash here)");*/
    
    globalSoloud.update3dAudio();

    /*m_3dFrameSkip++;
    if(m_3dFrameSkip >= 12){           // ← Try 4, 6, 8, 10
        globalSoloud.update3dAudio();
        m_3dFrameSkip = 0;
    }*/

    // After the for loop that does set3dSourcePosition
    /*m_3dUpdateTimer += Time::DeltaTime();  // assuming you have delta time
    if(m_3dUpdateTimer >= (1.0f/90.0f)){     // 90 Hz for 3D is enough
        globalSoloud.update3dAudio();
        m_3dUpdateTimer = 0.0f;
    }*/

    //LogInfo("Active voices: {}", soloud.getActiveVoiceCount());
}

}
#endif

#ifdef AUDIO_BACKEND_MINIAUDIO
#include <thread>

namespace OD{

ma_engine engine{};
bool hasInited = false;

std::thread::id mainThreadID;

inline bool IsMainThread(){
    return std::this_thread::get_id() == mainThreadID;
}

void AudioModuleInit(){
    AssetTypesDB::Get().RegisterAssetType<AudioClip>(".mp3", [](const std::string& path){ return AssetManager::Get().LoadAsset<AudioClip>(path); });
    AssetTypesDB::Get().RegisterAssetType<AudioClip>(".wav", [](const std::string& path){ return AssetManager::Get().LoadAsset<AudioClip>(path); });

    SceneManager::Get().RegisterCoreComponent<AudioSourceComponent>("AudioSourceComponent", "Audio");
    SceneManager::Get().RegisterSystem<AudioSystem>("AudioSystem");

    OD::GlobalSettings::Get().Register<AudioSettings>("Audio");

    mainThreadID = std::this_thread::get_id();

    if(!hasInited){
        ma_result result = ma_engine_init(nullptr, &engine);
        if(result != MA_SUCCESS){
            LogError("Failed to initialize miniaudio engine!");
            Assert(false);
            return;
        }
        hasInited = true;
        LogInfo("miniaudio initialized successfully");
    }
}

bool AudioSourceComponent::IsPlaying() const{
    Assert(IsMainThread() && "update3dAudio must be called from main thread!");
    if(clipHasInited == false) return false;
    return ma_sound_is_playing(&sourceSound);
}

void AudioSourceComponent::Play(){
    Assert(IsMainThread() && "update3dAudio must be called from main thread!");
    
    if(!clip || !clip->loaded){
        LogWarning("AudioSourceComponent::Play - No clip assigned.");
        return;
    }

    Stop();

    // Clone the sound (lightweight)
    ma_result result = ma_sound_init_copy(&engine, &clip->sound, 0, nullptr, &sourceSound);
    if(result != MA_SUCCESS) return;

    //ma_result result = ma_sound_init_from_data_source(&engine, &clip->buffer, 0, nullptr, &sourceSound);
    //if(result != MA_SUCCESS) return;

    ma_sound_set_looping(&sourceSound, loop);
    ma_sound_set_volume(&sourceSound, volume);
    ma_sound_set_pitch(&sourceSound, pitch);

    if(mode == AudioSourceMode::Mode3D){
        ma_sound_set_spatialization_enabled(&sourceSound, MA_TRUE);
        ma_sound_set_position(&sourceSound, position.x, position.y, position.z);
        ma_sound_set_min_distance(&sourceSound, minDistance);
        ma_sound_set_max_distance(&sourceSound, maxDistance);
        ma_sound_set_rolloff(&sourceSound, 1.0f); // adjust based on your attenuation enum
    } else {
        ma_sound_set_spatialization_enabled(&sourceSound, MA_FALSE);
    }

    auto result2 = ma_sound_start(&sourceSound);

    if(result2 != MA_SUCCESS){
        Assert(false);
    }

    clipHasInited = true;
}

void AudioSourceComponent::Stop(){
    Assert(IsMainThread() && "update3dAudio must be called from main thread!");
    if(clipHasInited == false) return;
    
    if(ma_sound_is_playing(&sourceSound)){
        ma_sound_stop(&sourceSound);
    }
    ma_sound_uninit(&sourceSound);
    clipHasInited = false;
}

void AudioSourceComponent::PlayOneShot(Ref<AudioClip> oneShotClip){
    Assert(IsMainThread() && "update3dAudio must be called from main thread!");

    if(!oneShotClip || !oneShotClip->loaded) return;

    const int maxShots = 10;
    if(oneShots.size() >= maxShots) return;

    //ma_sound temp;
    ma_sound* temp = new ma_sound;
    if(ma_sound_init_copy(&engine, &oneShotClip->sound, 0, nullptr, temp) != MA_SUCCESS) return;

    ma_sound_set_looping(temp, false);
    ma_sound_set_volume(temp, volume);
    ma_sound_set_pitch(temp, pitch);

    if(mode == AudioSourceMode::Mode3D){
        ma_sound_set_spatialization_enabled(temp, MA_TRUE);
        ma_sound_set_position(temp, position.x, position.y, position.z);
        ma_sound_set_min_distance(temp, minDistance);
        ma_sound_set_max_distance(temp, maxDistance);
    }

    auto result = ma_sound_start(temp);
    if(result != MA_SUCCESS){
        Assert(false);
    }

    oneShots.push_back(temp);
}

void AudioSourceComponent::SetPosition(const Vector3& pos){
    Assert(IsMainThread() && "update3dAudio must be called from main thread!");

    position = pos;
    if(clipHasInited == false) return;
    if(ma_sound_is_playing(&sourceSound) && mode == AudioSourceMode::Mode3D){
        ma_sound_set_position(&sourceSound, pos.x, pos.y, pos.z);
    }
}

void AudioSourceComponent::SetVolume(float vol){
    Assert(IsMainThread() && "update3dAudio must be called from main thread!");

    volume = std::clamp(vol, 0.0f, 2.0f);
    if(clipHasInited == false) return;
    ma_sound_set_volume(&sourceSound, volume);
}

void AudioSourceComponent::SetPitch(float p){
    Assert(IsMainThread() && "update3dAudio must be called from main thread!");

    pitch = std::clamp(p, 0.1f, 4.0f);
    if(clipHasInited == false) return;
    ma_sound_set_pitch(&sourceSound, pitch);
}

void AudioSourceComponent::SetLoop(bool l){
    Assert(IsMainThread() && "update3dAudio must be called from main thread!");

    loop = l;
    if(clipHasInited == false) return;
    ma_sound_set_looping(&sourceSound, loop);
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

void AudioSettings::OnImGuiRender(){
    ImGui::DragInt("maxActiveVoiceCount", &maxActiveVoiceCount, 0, 255);
    ImGui::DragFloat("volume", &volume, 1, 0, 2);
}

AudioSystem::AudioSystem(){
    Assert(IsMainThread() && "update3dAudio must be called from main thread!");
    name = "AudioSystem";
    //soloud.init(); 
    //Erro: call init twice on playing mode

    //ma_engine_start(&engine);
}

AudioSystem::~AudioSystem(){
    Assert(IsMainThread() && "update3dAudio must be called from main thread!");

    //ma_engine_stop(&engine);
    //ma_device_stop(engine.pDevice); // optional deeper stop
}

void AudioSystem::OnStop(Scene& scene){
    auto view = scene.GetRegistry().view<AudioSourceComponent>();
    for(auto [e, audio]: view.each()){
        if(audio.clipHasInited == true){
            //if(ma_sound_is_playing(&audio.sourceSound)) ma_sound_stop(&audio.sourceSound);
            ma_sound_uninit(&audio.sourceSound);
        }

        for(size_t i = 0; i < audio.oneShots.size(); i++){
            ma_sound* s = audio.oneShots[i];
            //if(ma_sound_is_playing(s)) ma_sound_stop(s);
            ma_sound_uninit(s);
            delete s;
        }
        audio.oneShots.clear();
    }
}

AudioSettings& AudioSystem::GetSettings(){
    return GlobalSettings::Get().Get<AudioSettings>();
}

void AudioSystem::UpdateSettings(){
    if(hasInited == false) return;
    Assert(IsMainThread() && "update3dAudio must be called from main thread!");

    const auto& settings = GlobalSettings::Get().Get<AudioSettings>();
}

void AudioSystem::Update(Scene& scene){
    OD_PROFILE_SCOPE("ScriptSystem::Update");
    Assert(IsMainThread() && "update3dAudio must be called from main thread!");

    // Update Listener (Camera)
    auto cam = scene.GetMainCamera();
    if(cam != EntityNull){
        TransformComponent& camTrans = scene.GetComponent<TransformComponent>(cam);
        Vector3 pos = camTrans.Position();
        Vector3 forward = camTrans.Back();
        Vector3 up = camTrans.Up();

        ma_engine_listener_set_position(&engine, 0, pos.x, pos.y, pos.z);
        ma_engine_listener_set_direction(&engine, 0, forward.x, forward.y, forward.z);
        ma_engine_listener_set_world_up(&engine, 0, up.x, up.y, up.z);
    }

    // Update Sources
    auto audioView = scene.GetRegistry().view<AudioSourceComponent, TransformComponent>();
    for(auto e : audioView){
        auto& audio = audioView.get<AudioSourceComponent>(e);

        if(audio.toPlay){
            audio.toPlay = false;
            audio.Play();
        }

        if(audio.mode == AudioSourceMode::Mode3D){
            TransformComponent& trans = audioView.get<TransformComponent>(e);
            Vector3 pos = trans.Position();
            audio.position = pos;

            if(audio.clipHasInited && ma_sound_is_playing(&audio.sourceSound)){
                ma_sound_set_position(&audio.sourceSound, pos.x, pos.y, pos.z);
            }
        }

        // CLEANUP one-shots
        for(size_t i = 0; i < audio.oneShots.size();){
            ma_sound* s = audio.oneShots[i];

            // if finished playing
            if(!ma_sound_is_playing(s)){
                ma_sound_uninit(s);
                delete s;

                audio.oneShots[i] = audio.oneShots.back();
                audio.oneShots.pop_back();
            } else {
                ++i;
            }
        }
    }
}

}
#endif

#ifdef AUDIO_BACKEND_NONE
#include <thread>

namespace OD{

ma_engine engine{};
bool hasInited = false;

std::thread::id mainThreadID;

inline bool IsMainThread(){
    return std::this_thread::get_id() == mainThreadID;
}

void AudioModuleInit(){
    AssetTypesDB::Get().RegisterAssetType<AudioClip>(".mp3", [](const std::string& path){ return AssetManager::Get().LoadAsset<AudioClip>(path); });
    AssetTypesDB::Get().RegisterAssetType<AudioClip>(".wav", [](const std::string& path){ return AssetManager::Get().LoadAsset<AudioClip>(path); });

    SceneManager::Get().RegisterCoreComponent<AudioSourceComponent>("AudioSourceComponent", "Audio");
    SceneManager::Get().RegisterSystem<AudioSystem>("AudioSystem");

    OD::GlobalSettings::Get().Register<AudioSettings>("Audio");

    mainThreadID = std::this_thread::get_id();

}

bool AudioSourceComponent::IsPlaying() const{
    Assert(IsMainThread() && "update3dAudio must be called from main thread!");
    return false;
}

void AudioSourceComponent::Play(){
    Assert(IsMainThread() && "update3dAudio must be called from main thread!");
}

void AudioSourceComponent::Stop(){
    Assert(IsMainThread() && "update3dAudio must be called from main thread!");
}

void AudioSourceComponent::PlayOneShot(Ref<AudioClip> oneShotClip){
    Assert(IsMainThread() && "update3dAudio must be called from main thread!");
}

void AudioSourceComponent::SetPosition(const Vector3& pos){
    Assert(IsMainThread() && "update3dAudio must be called from main thread!");
}

void AudioSourceComponent::SetVolume(float vol){
    Assert(IsMainThread() && "update3dAudio must be called from main thread!");
}

void AudioSourceComponent::SetPitch(float p){
    Assert(IsMainThread() && "update3dAudio must be called from main thread!");
}

void AudioSourceComponent::SetLoop(bool l){
    Assert(IsMainThread() && "update3dAudio must be called from main thread!");
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

void AudioSettings::OnImGuiRender(){
    ImGui::DragInt("maxActiveVoiceCount", &maxActiveVoiceCount, 0, 255);
    ImGui::DragFloat("volume", &volume, 1, 0, 2);
}

AudioSystem::AudioSystem(){
    Assert(IsMainThread() && "update3dAudio must be called from main thread!");
    name = "AudioSystem";
    //soloud.init(); 
    //Erro: call init twice on playing mode
}

AudioSystem::~AudioSystem(){
    Assert(IsMainThread() && "update3dAudio must be called from main thread!");
}

void AudioSystem::OnStop(Scene& scene){

}

AudioSettings& AudioSystem::GetSettings(){
    return GlobalSettings::Get().Get<AudioSettings>();
}

void AudioSystem::UpdateSettings(){
    if(hasInited == false) return;
    Assert(IsMainThread() && "update3dAudio must be called from main thread!");

    const auto& settings = GlobalSettings::Get().Get<AudioSettings>();
}

void AudioSystem::Update(Scene& scene){
    OD_PROFILE_SCOPE("ScriptSystem::Update");
    Assert(IsMainThread() && "update3dAudio must be called from main thread!");
}

}
#endif