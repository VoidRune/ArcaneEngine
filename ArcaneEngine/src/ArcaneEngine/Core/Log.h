#pragma once
#include <string_view>
#include <string>
#include <format>

namespace Arc
{
    void SetLogFile(const std::string& filename);

    template<typename... Args>
    void LogInfo(std::string_view format, Args&&... args);

    template<typename... Args>
    void LogWarning(std::string_view format, Args&&... args);

    template<typename... Args>
    void LogError(std::string_view format, Args&&... args);

    template<typename... Args>
    void LogFatal(std::string_view format, Args&&... args);
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