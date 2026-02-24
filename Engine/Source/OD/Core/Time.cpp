#include "OD/pch.h"
#include "Time.h"
#include "Math.h"

namespace OD{

float unscaledDeltaTime = 0;
float timeScale = 1;   
const float fixedDelta = 1.0f / 60.0f; // 60 Hz physics update

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
