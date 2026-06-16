#include "OD/pch.h"
#include "AudioClip.h"

namespace OD{

#ifdef AUDIO_BACKEND_MINIAUDIO
extern ma_engine engine;
#endif

AudioClip::AudioClip(const std::string& filePath){
    LoadFromFile(filePath);
}

AudioClip::~AudioClip(){
    #ifdef AUDIO_BACKEND_MINIAUDIO
    if(loaded) ma_sound_uninit(&sound);
    #endif 
    //LogInfo("DESTROYED: {}", path);
}

bool AudioClip::LoadFromFile(const std::string& path){
    #ifdef AUDIO_BACKEND_SOLOUD
    auto result = sample.load(path.c_str());
    if(result != SoLoud::SO_NO_ERROR){
        LogError("Erro to load: {}", path);
        return false;
    }

    this->path = path;

    return result == SoLoud::SO_NO_ERROR;
    #endif

    #ifdef AUDIO_BACKEND_MINIAUDIO
    ma_result result = ma_sound_init_from_file(
        &engine,                    // We'll use global engine later
        path.c_str(),
        MA_SOUND_FLAG_DECODE | MA_SOUND_FLAG_NO_SPATIALIZATION, // decode once
        nullptr,                    // pDataSource
        nullptr,                    // pDoneNotification
        &sound
    );

    if(result != MA_SUCCESS){
        LogError("Failed to load audio file: {}", path);
        return false;
    }

    /*ma_decoder_config dconfig = ma_decoder_config_init(
        ma_format_f32,  // FORCE FLOAT
        0,              // keep original channels
        0               // keep original sample rate
    );

    ma_decoder decoder;
    auto result2 = ma_decoder_init_file(path.c_str(), &dconfig, &decoder);
    Assert(result2 == MA_SUCCESS);

    //ma_decoder decoder;
    //auto result2 = ma_decoder_init_file(path.c_str(), nullptr, &decoder);
    //Assert(result2 == MA_SUCCESS);
    
    ma_uint64 frameCount;
    result2 = ma_decoder_get_length_in_pcm_frames(&decoder, &frameCount);
    Assert(result2 == MA_SUCCESS);
    //std::vector<float> pcm(frameCount * decoder.outputChannels);
    pcmData.resize(frameCount * decoder.outputChannels);
    result2 = ma_decoder_read_pcm_frames(&decoder, pcmData.data(), frameCount, nullptr);
    Assert(result2 == MA_SUCCESS);
    ma_audio_buffer_config config = ma_audio_buffer_config_init(
        ma_format_f32,
        decoder.outputChannels,
        frameCount,
        pcmData.data(),
        nullptr
    );
    result2 = ma_audio_buffer_init(&config, &buffer);
    Assert(result2 == MA_SUCCESS);

    LogInfo("Channels: {}", decoder.outputChannels);
    LogInfo("FrameCount: {}", frameCount);
    LogInfo("First sample: {}", pcmData[0]);

    result2 = ma_decoder_uninit(&decoder);
    Assert(result2 == MA_SUCCESS);

    if(result2 != MA_SUCCESS){
        LogError("Failed to load audio file: {}", path);
        return false;
    }*/

    this->path = path;
    this->loaded = true;
    return true;
    #endif

    return true;
}

bool AudioClip::LoadFromPackage(const std::string& path, Package& package){
    return false;

    void* data = nullptr;
    size_t size;
    if(package.ReadFileData(path.c_str(), data, size) == false){
        package.FreeFileData(data);
        return false;
    }   

    #ifdef AUDIO_BACKEND_SOLOUD
    auto result = sample.loadMem((unsigned char*)data, size, true);
    if(result != SoLoud::SO_NO_ERROR){
        LogError("Erro to load: {}", path);
        package.FreeFileData(data);
        return false;
    }

    this->path = path;

    package.FreeFileData(data);
    return result == SoLoud::SO_NO_ERROR;
    #endif

    /*#ifdef AUDIO_BACKEND_MINIAUDIO
    // Create a memory data source
    ma_sound tmpSound{};
    ma_result result = ma_sound_init_from_memory(
        nullptr,                    // engine (set later)
        data,
        size,
        MA_SOUND_FLAG_DECODE | MA_SOUND_FLAG_NO_SPATIALIZATION,
        nullptr,
        nullptr,
        &tmpSound
    );

    if(result != MA_SUCCESS){
        LogError("Failed to load audio from memory: {}", path);
        package.FreeFileData(data);
        return false;
    }

    // Move to our sound
    ma_sound_uninit(&sample);
    sample = tmpSound;

    this->path = path;
    this->loaded = true;

    package.FreeFileData(data);
    return true;
    #endif*/

    return true;
}

std::vector<std::string> AudioClip::GetFileAssociations(){
    return std::vector<std::string>{
        ".mp3", ".wav"
    };
}

}