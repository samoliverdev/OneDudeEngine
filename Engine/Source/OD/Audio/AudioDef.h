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
#define AUDIO_CLIP_DATA ma_sound sound; ma_audio_buffer buffer; std::vector<float> pcmData;
#define AUDIO_COMP_DATA ma_sound sourceSound{}; bool clipHasInited = false; std::vector<ma_sound*> oneShots; // Per-instance sound
#endif

#ifdef AUDIO_BACKEND_NONE
#include <miniaudio.h>
#define AUDIO_CLIP_DATA 
#define AUDIO_COMP_DATA 
#endif