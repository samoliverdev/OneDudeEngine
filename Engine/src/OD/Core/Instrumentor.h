#pragma once

#include "OD/Defines.h"
#include <string>
#include <chrono>
#include <thread>
#include <functional>

namespace OD{

struct OD_API ProfileResult{
    const char* name;
    long long start, end;
    uint32_t threadID;
    int parent = -1;
};

class OD_API Instrumentor{
    friend class InstrumentationTimer;
public:
    static void BeginLoop();
    static void EndLoop();

    static const std::vector<ProfileResult>& Results();
    //static Instrumentor& Get();
//private:
    //std::vector<ProfileResult> results;
    //std::vector<int> nodesStack;
};

class OD_API InstrumentationTimer{
public:
    InstrumentationTimer(const char* _name);
    ~InstrumentationTimer();
    void Stop();
private:
    const char* name;
    int index;
    std::chrono::time_point<std::chrono::high_resolution_clock> startTimepoint;
    bool stopped = false;
};

class OD_API SimpleTimer{
public:
    SimpleTimer(std::function<void(float)> _onStop);
    ~SimpleTimer();
    void Stop();
private:
    std::function<void(float)> onStop;
    std::chrono::time_point<std::chrono::high_resolution_clock> startTimepoint;
    bool stopped = false;
};

}

#if OD_PROFILE
#define OD_PROFILE_SCOPE(name) ::OD::InstrumentationTimer timer##__LINE__(name);
#define OD_PROFILE_FUNCTION() OD_PROFILE_SCOPE(__FUNCSIG__)
#define OD_LOG_PROFILE(name) SimpleTimer _name([](float duration){ LogWarning("%s: %.3f.ms", name, duration);});
#define OD_LOG_PROFILE2(name, str) SimpleTimer _name([](float duration){ LogWarning("%s: %.3f.ms", str, duration);});
#else
#define OD_PROFILE_SCOPE(name)
#define OD_PROFILE_FUNCTION()
#endif