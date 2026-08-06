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

struct OneShotDecompressData{
    ma_sound* sound;
    ma_audio_buffer_ref* ref;
};

struct OneShotStreamingData{
    ma_sound* sound;
    ma_decoder* decoder;
};

#define AUDIO_CLIP_DATA AudioClipDecompressData data1;
#define AUDIO_COMP_DATA ma_sound sourceSound{}; ma_audio_buffer_ref sourceBufferRef{}; bool clipHasInited = false; std::vector<OneShotDecompressData> oneShots1; std::vector<OneShotStreamingData> oneShots2; // Per-instance sound
#endif

#ifdef AUDIO_BACKEND_NONE
#include <miniaudio.h>
#define AUDIO_CLIP_DATA 
#define AUDIO_COMP_DATA 
#endif