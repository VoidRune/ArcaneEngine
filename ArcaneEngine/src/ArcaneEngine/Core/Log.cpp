#include "Log.h"
#include <iostream>
#include <fstream>
#include <chrono>
#include <mutex>
#include <format>

namespace Arc
{
    std::ofstream logFile;
    std::mutex logMutex;

    void SetLogFile(const std::string& filename)
    {
        logFile.open(filename, std::ios::app);
        if (!logFile)
        {
            std::cerr << "[ERROR] Failed to open log file: " << filename << std::endl;
        }
    }

    std::string GetTimeStamp()
    {
        auto now = std::chrono::system_clock::now();
        auto timeT = std::chrono::system_clock::to_time_t(now);
        tm localTime{};
        localtime_s(&localTime, &timeT);

        char buffer[20];
        strftime(buffer, sizeof(buffer), "%d-%m-%Y %H:%M:%S", &localTime);
        return std::string(buffer);
    }

    template<typename... Args>
    void LogMessage(const std::string& level, std::string_view format, Args&&... args)
    {
        std::lock_guard<std::mutex> lock(logMutex);  // Prevent race conditions

        std::string timestamp = GetTimeStamp();
        std::string message = std::vformat(format, std::make_format_args(args...));

        std::cout << "[" << timestamp << "] " << level << ": " << message << std::endl;
        if (logFile.is_open())
        {
            logFile << "[" << timestamp << "] " << level << ": " << message << std::endl;
        }
    }

    template<typename... Args>
    void LogInfo(std::string_view format, Args&&... args)
    {
        LogMessage("INFO", format, std::forward<Args>(args)...);
    }

    template<typename... Args>
    void LogWarning(std::string_view format, Args&&... args)
    {
        LogMessage("WARN", format, std::forward<Args>(args)...);
    }

    template<typename... Args>
    void LogError(std::string_view format, Args&&... args)
    {
        LogMessage("ERROR", format, std::forward<Args>(args)...);
    }

    template<typename... Args>
    void LogFatal(std::string_view format, Args&&... args)
    {
        LogMessage("FATAL", format, std::forward<Args>(args)...);
    }

    // Explicit template instantiations to prevent linker issues
    template void LogInfo<>(std::string_view);
    template void LogWarning<>(std::string_view);
    template void LogError<>(std::string_view);
    template void LogFatal<>(std::string_view);
}