#pragma once
#include "OD/Defines.h"

namespace OD{

class OD_API Time{
    friend class Application;
public:
    static float DeltaTime();
    static float UnscaledDeltaTime();
    static float TimeScale();
    static void TimeScale(float v);
private:
    static void UnscaledDeltaTime(float v);
};

}