#pragma once

//#define AUDIO_BACKEND_SOLOUD
#define AUDIO_BACKEND_MINIAUDIO

#ifdef AUDIO_BACKEND_SOLOUD
#include <soloud_wav.h>
#include <soloud.h>
#define AUDIO_CLIP_DATA SoLoud::Wav sample;
#define AUDIO_COMP_DATA SoLoud::Soloud* soloud = nullptr; SoLoud::handle handle = 0;
#endif

#ifdef AUDIO_BACKEND_MINIAUDIO
#include <miniaudio.h>
#define AUDIO_CLIP_DATA ma_sound sound; 
#define AUDIO_COMP_DATA ma_sound sourceSound{}; bool hasInited = false; std::vector<ma_sound*> oneShots; // Per-instance sound
#endif