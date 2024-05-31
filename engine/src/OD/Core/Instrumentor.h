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
};

class OD_API Instrumentor{
public:
    std::vector<ProfileResult>& Results();
    void WriteProfile(const ProfileResult& _result);
    static Instrumentor& Get();
private:
    std::vector<ProfileResult> results;
};

class OD_API InstrumentationTimer{
public:
    InstrumentationTimer(const char* _name);
    ~InstrumentationTimer();
    void Stop();
private:
    const char* name;
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
#else
#define OD_PROFILE_SCOPE(name)
#define OD_PROFILE_FUNCTION()
#endif