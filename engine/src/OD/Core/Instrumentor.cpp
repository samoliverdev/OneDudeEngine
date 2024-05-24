#include "Instrumentor.h"

namespace OD{

std::vector<ProfileResult>& Instrumentor::results(){ return _results; }

void Instrumentor::WriteProfile(const ProfileResult& result){
    _results.push_back(result);
    //LogInfo("Instrumentor::WriteProfile: %s", result.name);
}

Instrumentor& Instrumentor::Get(){
    static Instrumentor instance;
    return instance;
}

InstrumentationTimer::InstrumentationTimer(const char* name): _name(name), _stopped(false){
    _startTimepoint = std::chrono::high_resolution_clock::now();
}

InstrumentationTimer::~InstrumentationTimer(){
    if (!_stopped) Stop();
}

void InstrumentationTimer::Stop(){
    auto endTimepoint = std::chrono::high_resolution_clock::now();

    long long start = std::chrono::time_point_cast<std::chrono::microseconds>(_startTimepoint).time_since_epoch().count();
    long long end = std::chrono::time_point_cast<std::chrono::microseconds>(endTimepoint).time_since_epoch().count();

    uint32_t threadID = std::hash<std::thread::id>{}(std::this_thread::get_id());
    //Instrumentor::Get().WriteProfile({ _name, start, end, threadID });
    Instrumentor::Get().WriteProfile({ _name, start, end, threadID });

    _stopped = true;
}

}