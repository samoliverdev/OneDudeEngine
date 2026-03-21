#pragma once
#include "OD/Defines.h"
#include <memory>
#include <spdlog/spdlog.h>

namespace OD{

class OD_API Log{
    friend class Application;
public:
    enum class Level { Info, Warning, Error, Fatal };

    struct OD_API LogEntry{
        Level level;
        std::string message;
    };

    static std::shared_ptr<spdlog::logger>& GetLogger();
    static void DrainQueue();
    static const std::vector<LogEntry>& GetEntries();
    static void EntriesClear();

private:
    static void Init();
    static void Shutdown();
};

}

#if not defined(FINAL_BUILD) || defined(USE_LOG_ON_FINAL_BUILD)
    #define LogInfo(...) ::OD::Log::GetLogger()->info(__VA_ARGS__)
    #define LogWarning(...) ::OD::Log::GetLogger()->warn(__VA_ARGS__)
    #define LogError(...) ::OD::Log::GetLogger()->error(__VA_ARGS__)
    #define LogFatal(...) ::OD::Log::GetLogger()->critical(__VA_ARGS__)

    #define LogInfoExtra(...) ::OD::Log::GetLogger()->info(__VA_ARGS__)
    #define LogWarningExtra(...) ::OD::Log::GetLogger()->warn(__VA_ARGS__)
    #define LogErrorExtra(...) ::OD::Log::GetLogger()->error(__VA_ARGS__)
    #define LogFatalExtra(...) ::OD::Log::GetLogger()->critical(__VA_ARGS__)
#else
    #define LogInfo(...)
	#define LogWarning(...)
	#define LogError(...)
    #define LogFatal(...)

    #define LogInfoExtra(...)
	#define LogWarningExtra(...)
	#define LogErrorExtra(...)
    #define LogFatalExtra(...)
#endif