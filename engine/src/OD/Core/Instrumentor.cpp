#include "Instrumentor.h"

namespace OD{

std::vector<ProfileResult>& Instrumentor::Results(){ return results; }

void Instrumentor::WriteProfile(const ProfileResult& result){
    results.push_back(result);
}

Instrumentor& Instrumentor::Get(){
    static Instrumentor instance;
    return instance;
}

InstrumentationTimer::InstrumentationTimer(const char* _name): name(_name), stopped(false){
    startTimepoint = std::chrono::high_resolution_clock::now();
}

InstrumentationTimer::~InstrumentationTimer(){
    if(!stopped) Stop();
}

void InstrumentationTimer::Stop(){
    auto endTimepoint = std::chrono::high_resolution_clock::now();

    long long start = std::chrono::time_point_cast<std::chrono::microseconds>(startTimepoint).time_since_epoch().count();
    long long end = std::chrono::time_point_cast<std::chrono::microseconds>(endTimepoint).time_since_epoch().count();

    uint32_t threadID = std::hash<std::thread::id>{}(std::this_thread::get_id());
    //Instrumentor::Get().WriteProfile({ _name, start, end, threadID });
    Instrumentor::Get().WriteProfile({ name, start, end, threadID });

    stopped = true;
}

SimpleTimer::SimpleTimer(std::function<void(float)> _onStop): onStop(_onStop), stopped(false){
    startTimepoint = std::chrono::high_resolution_clock::now();
}

SimpleTimer::~SimpleTimer(){
    if(!stopped) Stop();
}

void SimpleTimer::Stop(){
    auto endTimepoint = std::chrono::high_resolution_clock::now();

    long long start = std::chrono::time_point_cast<std::chrono::microseconds>(startTimepoint).time_since_epoch().count();
    long long end = std::chrono::time_point_cast<std::chrono::microseconds>(endTimepoint).time_since_epoch().count();

    uint32_t threadID = std::hash<std::thread::id>{}(std::this_thread::get_id());
    //Instrumentor::Get().WriteProfile({ _name, start, end, threadID });
    //Instrumentor::Get().WriteProfile({ name, start, end, threadID });
    
    float durration = (end - start) * 0.001f;
    onStop(durration);

    stopped = true;
}

}