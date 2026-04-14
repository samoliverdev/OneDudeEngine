#include "OD/pch.h"
#include "Instrumentor.h"
#include "OD/Graphics/Graphics.h"

namespace OD{

std::vector<ProfileResult> last;
std::vector<ProfileResult> results;
std::vector<int> nodesStack;

std::vector<GpuProfileResult> lastGpu;
std::vector<GpuProfileResult> resultsGpu;
std::vector<int> nodesStackGpu;

void Instrumentor::BeginLoop(){
    results.clear();
    nodesStack.clear();

    resultsGpu.clear();
    nodesStackGpu.clear();
}

void Instrumentor::EndLoop(){
    last = results;

    lastGpu = resultsGpu;
}

const std::vector<ProfileResult>& Instrumentor::Results(){ return last; }
const std::vector<GpuProfileResult>& Instrumentor::ResultsGpu(){ return lastGpu; }

InstrumentationTimer::InstrumentationTimer(const char* _name): name(_name), stopped(false){
    startTimepoint = std::chrono::high_resolution_clock::now();

    results.push_back({
        0, 0, 0, nodesStack.empty() ? -1 : nodesStack[nodesStack.size()-1], _name
    });
    nodesStack.push_back(results.size()-1);
    index = results.size()-1;
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
    //Instrumentor::Get().WriteProfile({ name, start, end, threadID });

    results[index].start = start;
    results[index].end = end;
    results[index].threadID = threadID;
    nodesStack.pop_back();

    stopped = true;
}

GpuInstrumentationTimer::GpuInstrumentationTimer(const char* _name): name(_name), stopped(false){
    resultsGpu.push_back({
        0, nodesStackGpu.empty() ? -1 : nodesStackGpu[nodesStackGpu.size()-1], _name
    });
    nodesStackGpu.push_back(resultsGpu.size()-1);
    index = resultsGpu.size()-1;

    Graphics::BeginGPUTime();
}

GpuInstrumentationTimer::~GpuInstrumentationTimer(){
    if(!stopped) Stop();
}

void GpuInstrumentationTimer::Stop(){
    resultsGpu[index].time = Graphics::EndGPUTime();
    nodesStackGpu.pop_back();
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