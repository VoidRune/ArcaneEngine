#pragma once
#include <string>

namespace Arc
{
	void SetLogToFile(const std::string& fileName);
	void SetLogToConsole(bool setLogToConsole);
	void LogInfo(const std::string& msg);
	void LogWarning(const std::string& msg);
	void LogError(const std::string& msg);
}

#if defined DEBUG || defined RELEASE
#define ARC_SET_LOG_TO_FILE(...) Arc::SetLogToFile(__VA_ARGS__)
#define ARC_LOG(...) Arc::LogInfo(__VA_ARGS__)
#define ARC_LOG_WARNING(...) Arc::LogWarning(__VA_ARGS__)
#define ARC_LOG_ERROR(...) Arc::LogError(__VA_ARGS__)
#else
#define ARC_SET_LOG_TO_FILE(...)
#define ARC_LOG()
#define ARC_LOG_WARNING(...)
#define ARC_LOG_ERROR(...)
#endif // DEBUG || RELEASE
