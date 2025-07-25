#include "Time.h"
#include "Math.h"

namespace OD{

float unscaledDeltaTime = 0;
float timeScale = 1;   

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

} 
