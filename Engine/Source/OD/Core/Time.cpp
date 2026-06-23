#include "OD/pch.h"
#include "Time.h"
#include "Math.h"

namespace OD{

float time = 0.0f;          // scaled time
float unscaledTime = 0.0f; // real elapsed time

float unscaledDeltaTime = 0;
float timeScale = 1;   
const float fixedDelta = 1.0f / 60.0f; // 60 Hz physics update

void Time::Update(float dt){
    float _unscaledDeltaTime = dt;
    float _deltaTime = dt * timeScale;

    unscaledTime += _unscaledDeltaTime;
    time += _deltaTime;
}

float Time::TimeSinceStartup(){
    return time;
}

float Time::UnscaledTime(){ 
    return unscaledTime; 
} 

float Time::DeltaTime(){
    return unscaledDeltaTime * timeScale;
}

float Time::UnscaledDeltaTime(){
    return unscaledDeltaTime;
}

float Time::TimeScale(){
    return timeScale;
}

void Time::TimeScale(float v){
    timeScale = math::max<float>(0, v);
}

void Time::UnscaledDeltaTime(float v){
    unscaledDeltaTime = v;
}

float Time::FixedDelta(){
    #if ENABLE_FIXED
    return fixedDelta * timeScale;
    #else
    return DeltaTime();
    #endif
}

float Time::UnscaledFixedDelta(){
    #if ENABLE_FIXED
    return fixedDelta;
    #else
    return UnscaledDeltaTime();
    #endif
}

} 
