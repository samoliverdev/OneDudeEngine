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
private:
    std::vector<ProfileResult> _results;
public:
    std::vector<ProfileResult>& results(){ return _results; }

    void WriteProfile(const ProfileResult& result){
        _results.push_back(result);
    }

    static Instrumentor& Get(){
        static Instrumentor instance;
        return instance;
    }
};

class OD_API InstrumentationTimer{
public:
    InstrumentationTimer(const char* name): _name(name), _stopped(false){
        _startTimepoint = std::chrono::high_resolution_clock::now();
    }

    ~InstrumentationTimer(){
        if (!_stopped) Stop();
    }

    void Stop(){
        auto endTimepoint = std::chrono::high_resolution_clock::now();

        long long start = std::chrono::time_point_cast<std::chrono::microseconds>(_startTimepoint).time_since_epoch().count();
        long long end = std::chrono::time_point_cast<std::chrono::microseconds>(endTimepoint).time_since_epoch().count();

        uint32_t threadID = std::hash<std::thread::id>{}(std::this_thread::get_id());
        //Instrumentor::Get().WriteProfile({ _name, start, end, threadID });
        Instrumentor::Get().WriteProfile({ _name, start, end, threadID });

        _stopped = true;
    }
private:
    const char* _name;
    std::chrono::time_point<std::chrono::high_resolution_clock> _startTimepoint;
    bool _stopped;
};

}

#if OD_PROFILE
#define OD_PROFILE_SCOPE(name) ::OD::InstrumentationTimer timer##__LINE__(name);
#define OD_PROFILE_FUNCTION() OD_PROFILE_SCOPE(__FUNCSIG__)
#else
#define OD_PROFILE_SCOPE(name)
#define OD_PROFILE_FUNCTION()
#endif