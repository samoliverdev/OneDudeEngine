#pragma once

//#define AUDIO_BACKEND_SOLOUD
#define AUDIO_BACKEND_MINIAUDIO
//#define AUDIO_BACKEND_NONE

#ifdef AUDIO_BACKEND_SOLOUD
#include <soloud_wav.h>
#include <soloud.h>
#define AUDIO_CLIP_DATA SoLoud::Wav sample;
#define AUDIO_COMP_DATA SoLoud::Soloud* soloud = nullptr; SoLoud::handle handle = 0;
#endif

#ifdef AUDIO_BACKEND_MINIAUDIO
#include <miniaudio.h>

struct AudioClipDecompressData{
    ma_audio_buffer buffer;
    ma_format format;
    ma_uint32 channels;
    ma_uint32 sampleRate;
    std::vector<float> pcm; // owned decoded data
};

struct AudioClipStreamingData{
    std::vector<uint8_t> memory; // for package
};

struct OneShotData{
    ma_sound* sound;
    ma_audio_buffer_ref* ref = nullptr;
    ma_decoder* decoder = nullptr;
};

#define DEFAULT_DECODE_CONFIG_INIT() ma_decoder_config_init(ma_format_f32, 2, 44100)

#define AUDIO_CLIP_DATA AudioClipDecompressData data1; AudioClipStreamingData data2;
#define AUDIO_COMP_DATA ma_sound sourceSound{}; ma_audio_buffer_ref sourceBufferRef{}; ma_decoder decoder{}; bool clipHasInited = false; std::vector<OneShotData> oneShots1;
#endif

#ifdef AUDIO_BACKEND_NONE
#include <miniaudio.h>
#define AUDIO_CLIP_DATA 
#define AUDIO_COMP_DATA 
#endif