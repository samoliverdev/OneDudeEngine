#pragma once
#include "OD/Defines.h"
#include <memory>
#include <spdlog/spdlog.h>

namespace OD{
namespace Log{
    OD_API std::shared_ptr<spdlog::logger>& GetLogger();
    OD_API void Init();
    OD_API void Shutdown();

    enum class Level { Info, Warning, Error, Fatal };

    struct OD_API LogEntry{
        Level level;
        std::string message;
    };

    OD_API void DrainQueue();
    OD_API const std::vector<LogEntry>& GetEntries();
    OD_API void EntriesClear();
}
}

#ifndef FINAL_BUILD
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