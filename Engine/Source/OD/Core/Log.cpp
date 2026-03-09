#include "OD/pch.h"
#include "Log.h"
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <concurrentqueue.h>

namespace OD{
 
std::shared_ptr<spdlog::logger> logger = nullptr;
moodycamel::ConcurrentQueue<Log::LogEntry> logQueue;
std::vector<Log::LogEntry> entries;
constexpr size_t MaxLogCount = 2000;

class LogQueueSink final : public spdlog::sinks::base_sink<std::mutex>{
protected:
    void sink_it_(const spdlog::details::log_msg& msg) override{
        Log::LogEntry e;
        e.message = std::string(msg.payload.data(), msg.payload.size());

        switch(msg.level){
            case spdlog::level::warn:     e.level = Log::Level::Warning; break;
            case spdlog::level::err:      e.level = Log::Level::Error;   break;
            case spdlog::level::critical: e.level = Log::Level::Fatal;   break;
            default:                      e.level = Log::Level::Info;    break;
        }

        logQueue.enqueue(std::move(e));
    }

    void flush_() override {}
};

void Log::Init(){
    //std::printf("--------------Log Init--------------\n");
    std::vector<spdlog::sink_ptr> sinks;
    sinks.push_back(std::make_shared<spdlog::sinks::stdout_color_sink_mt>());
    sinks.push_back(std::make_shared<spdlog::sinks::basic_file_sink_mt>("Engine.log", true));
    sinks.push_back(std::make_shared<LogQueueSink>());

    logger = std::make_shared<spdlog::logger>("ENGINE", sinks.begin(), sinks.end());
    spdlog::register_logger(logger);

    logger->set_level(spdlog::level::trace);
    logger->set_pattern("[%H:%M:%S] [%^%l%$] %v");
}

void Log::Shutdown(){
    logger.reset();
    spdlog::shutdown();
}

std::shared_ptr<spdlog::logger>& Log::GetLogger(){
    return logger;
}

void Log::DrainQueue(){
    Log::LogEntry e;
    while(logQueue.try_dequeue(e)){
        if(entries.size() >= MaxLogCount) entries.erase(entries.begin());
        entries.emplace_back(std::move(e));
    }
}

const std::vector<Log::LogEntry>& Log::GetEntries(){
    return entries;
}

void Log::EntriesClear(){
    entries.clear();
}

}
