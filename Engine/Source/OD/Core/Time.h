#pragma once
#include "OD/Defines.h"

#define ENABLE_FIXED 1

namespace OD{

class OD_API Time{
    friend class Application;
public:
    static float DeltaTime();
    static float UnscaledDeltaTime();
    static float TimeScale();
    static void TimeScale(float v);
    static float FixedDelta(); 
    static float UnscaledFixedDelta();  
private:
    static void UnscaledDeltaTime(float v);
};

}