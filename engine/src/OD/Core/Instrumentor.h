#pragma once

#include "OD/Defines.h"
#include <string>
#include <chrono>
#include <thread>

namespace OD{

struct OD_API ProfileResult{
    const char* name;
    long long start, end;
    uint32_t threadID;
};

class OD_API Instrumentor{
public:
    std::vector<ProfileResult>& results();
    void WriteProfile(const ProfileResult& result);
    static Instrumentor& Get();
private:
    std::vector<ProfileResult> _results;
};

class OD_API InstrumentationTimer{
public:
    InstrumentationTimer(const char* name);
    ~InstrumentationTimer();
    void Stop();
private:
    const char* _name;
    std::chrono::time_point<std::chrono::high_resolution_clock> _startTimepoint;
    bool _stopped = false;
};

}

#if OD_PROFILE
#define OD_PROFILE_SCOPE(name) ::OD::InstrumentationTimer timer##__LINE__(name);
#define OD_PROFILE_FUNCTION() OD_PROFILE_SCOPE(__FUNCSIG__)
#else
#define OD_PROFILE_SCOPE(name)
#define OD_PROFILE_FUNCTION()
#endif