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
    if(loaded){
        //ma_sound_uninit(&sound);
        //ma_decoder_uninit(&decoder);

        ma_audio_buffer_uninit(&data1.buffer);
    }
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
    if(loadType == AudioClipLoadType::DecompressOnLoad){
        ma_decoder decoder;
        ma_decoder_config config = ma_decoder_config_init(ma_format_f32, 0, 0);
        if(ma_decoder_init_file(path.c_str(), &config, &decoder) != MA_SUCCESS) return false;

        ma_uint64 totalFrames = 0;
        if(ma_decoder_get_length_in_pcm_frames(&decoder, &totalFrames) != MA_SUCCESS){
            ma_decoder_uninit(&decoder);
            return false;
        }

        //data1.pcm.resize((size_t)(totalFrames * decoder.outputChannels));

        size_t sampleSize = ma_get_bytes_per_sample(decoder.outputFormat);
        data1.pcm.resize((size_t)(totalFrames * decoder.outputChannels * sampleSize));

        ma_decoder_read_pcm_frames(
            &decoder,
            data1.pcm.data(),
            totalFrames,
            nullptr
        );

        data1.format = decoder.outputFormat;
        data1.channels = decoder.outputChannels;
        data1.sampleRate = decoder.outputSampleRate;

        ma_audio_buffer_config bufferConfig = ma_audio_buffer_config_init(data1.format, data1.channels, totalFrames, data1.pcm.data(), nullptr);
        if(ma_audio_buffer_init(&bufferConfig, &data1.buffer) != MA_SUCCESS){
            ma_audio_buffer_uninit(&data1.buffer);
            ma_decoder_uninit(&decoder);
            return false;
        }

        ma_decoder_uninit(&decoder);

        Assert(data1.pcm.size() > 0);
        Assert(totalFrames > 0);

        loaded = true;
        return true;
    }

    if(loadType == AudioClipLoadType::Streaming){
        Assert(false);
    }

    #endif

    return true;
}

bool AudioClip::LoadFromPackage(const std::string& path, Package& package){
    //return false;

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

    #ifdef AUDIO_BACKEND_MINIAUDIO
    if(loadType == AudioClipLoadType::DecompressOnLoad){
        ma_decoder decoder;
        ma_decoder_config config = ma_decoder_config_init(ma_format_f32, 0, 0);

        if(ma_decoder_init_memory(data, size, &config, &decoder) != MA_SUCCESS) return false;

        ma_uint64 totalFrames;
        if(ma_decoder_get_length_in_pcm_frames(&decoder, &totalFrames) != MA_SUCCESS){
            ma_decoder_uninit(&decoder);
            return false;
        }

        data1.pcm.resize((size_t)(totalFrames * decoder.outputChannels));

        ma_decoder_read_pcm_frames(
            &decoder,
            data1.pcm.data(),
            totalFrames,
            nullptr
        );

        data1.format = decoder.outputFormat;
        data1.channels = decoder.outputChannels;
        data1.sampleRate = decoder.outputSampleRate;

        ma_audio_buffer_config bufferConfig = ma_audio_buffer_config_init(data1.format, data1.channels, totalFrames, data1.pcm.data(), nullptr);

        if(ma_audio_buffer_init(&bufferConfig, &data1.buffer) != MA_SUCCESS){
            ma_audio_buffer_uninit(&data1.buffer);
            ma_decoder_uninit(&decoder);
            return false;
        }

        ma_decoder_uninit(&decoder);

        loaded = true;
        return true;
    }
    #endif

    return true;
}

std::vector<std::string> AudioClip::GetFileAssociations(){
    return std::vector<std::string>{
        ".mp3", ".wav"
    };
}

}