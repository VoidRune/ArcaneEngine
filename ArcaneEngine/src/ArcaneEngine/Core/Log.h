#pragma once
#include <string>
#include <format>

namespace Arc
{
    void SetLogFile(const std::string& filename);
    void LogMessageInternal(const std::string& level, const std::string& message);

    template<typename... Args>
    inline void LogMessage(const std::string& level, std::string_view format, Args&&... args)
    {
        std::string message = std::vformat(format, std::make_format_args(args...));
        LogMessageInternal(level, message);
    }

    template<typename... Args>
    inline void LogInfo(std::string_view format, Args&&... args)
    {
        LogMessage("INFO", format, std::forward<Args>(args)...);
    }

    template<typename... Args>
    inline void LogWarning(std::string_view format, Args&&... args)
    {
        LogMessage("WARN", format, std::forward<Args>(args)...);
    }

    template<typename... Args>
    inline void LogError(std::string_view format, Args&&... args)
    {
        LogMessage("ERROR", format, std::forward<Args>(args)...);
    }

    template<typename... Args>
    inline void LogFatal(std::string_view format, Args&&... args)
    {
        LogMessage("FATAL", format, std::forward<Args>(args)...);
    }
}

#if defined(DEBUG) || defined(RELEASE)
#define ARC_LOG(...) Arc::LogInfo(__VA_ARGS__)
#define ARC_LOG_WARNING(...) Arc::LogWarning(__VA_ARGS__)
#define ARC_LOG_ERROR(...) Arc::LogError(__VA_ARGS__)
#define ARC_LOG_FATAL(...) Arc::LogFatal(__VA_ARGS__)
#else
#define ARC_LOG(...)
#define ARC_LOG_WARNING(...)
#define ARC_LOG_ERROR(...)
#define ARC_LOG_FATAL(...)
#endif