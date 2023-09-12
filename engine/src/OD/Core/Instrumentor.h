//
// Basic instrumentation profiler by Cherno

// Usage: include this header file somewhere in your code (eg. precompiled header), and then use like:
//
// Instrumentor::Get().BeginSession("Session Name");        // Begin session 
// {
//     InstrumentationTimer timer("Profiled Scope Name");   // Place code like this in scopes you'd like to include in profiling
//     // Code
// }
// Instrumentor::Get().EndSession();                        // End Session
//
// You will probably want to macro-fy this, to switch on/off easily and use things like __FUNCSIG__ for the profile name.
//
#pragma once

#include "OD/Defines.h"
#include <string>
#include <chrono>
#include <algorithm>
#include <fstream>
#include <thread>

namespace OD{

struct ProfileResult{
    std::string name;
    long long start, end;
    uint32_t threadID;
};

struct InstrumentationSession{
    std::string name;
};

class Instrumentor{
private:
    InstrumentationSession* _currentSession;
    std::ofstream _outputStream;
    int _profileCount;
public:
    Instrumentor(): _currentSession(nullptr), _profileCount(0){}

    void BeginSession(const std::string& name, const std::string& filepath = "results.json"){
        _outputStream.open(filepath);
        WriteHeader();
        _currentSession = new InstrumentationSession{ name };
    }

    void EndSession(){
        WriteFooter();
        _outputStream.close();
        delete _currentSession;
        _currentSession = nullptr;
        _profileCount = 0;
    }

    void WriteProfile(const ProfileResult& result){
        if (_profileCount++ > 0)
            _outputStream << ",";

        std::string name = result.name;
        std::replace(name.begin(), name.end(), '"', '\'');

        _outputStream << "{";
        _outputStream << "\"cat\":\"function\",";
        _outputStream << "\"dur\":" << (result.end - result.start) << ',';
        _outputStream << "\"name\":\"" << name << "\",";
        _outputStream << "\"ph\":\"X\",";
        _outputStream << "\"pid\":0,";
        _outputStream << "\"tid\":" << result.threadID << ",";
        _outputStream << "\"ts\":" << result.start;
        _outputStream << "}";

        _outputStream.flush();
    }

    void WriteHeader(){
        _outputStream << "{\"otherData\": {},\"traceEvents\":[";
        _outputStream.flush();
    }

    void WriteFooter(){
        _outputStream << "]}";
        _outputStream.flush();
    }

    static Instrumentor& Get(){
        static Instrumentor instance;
        return instance;
    }
};

class InstrumentationTimer{
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
#define OD_PROFILE_BEGIN_SESSION(name, filepath) ::OD::Instrumentor::Get().BeginSession(name, filepath)
#define OD_PROFILE_END_SESSION() ::OD::Instrumentor::Get().EndSession()
#define OD_PROFILE_SCOPE(name) ::OD::InstrumentationTimer timer##__LINE__(name);
#define OD_PROFILE_FUNCTION() OD_PROFILE_SCOPE(__FUNCSIG__)
#else
#define OD_PROFILE_BEGIN_SESSION(name, filepath)
#define OD_PROFILE_END_SESSION()
#define OD_PROFILE_SCOPE(name)
#define OD_PROFILE_FUNCTION()
#endif