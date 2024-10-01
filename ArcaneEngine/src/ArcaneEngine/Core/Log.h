#pragma once
#include <string>

namespace Arc
{
	void LogInfo(const std::string& msg);
	void LogWarning(const std::string& msg);
	void LogError(const std::string& msg);
	void LogFatal(const std::string& msg);
}

#if defined DEBUG || defined RELEASE
#define ARC_LOG(...) Arc::LogInfo(__VA_ARGS__)
#define ARC_LOG_WARNING(...) Arc::LogWarning(__VA_ARGS__)
#define ARC_LOG_ERROR(...) Arc::LogError(__VA_ARGS__)
#define ARC_LOG_FATAL(...) Arc::LogFatal(__VA_ARGS__)
#else
#define ARC_LOG()
#define ARC_LOG_WARNING(...)
#define ARC_LOG_ERROR(...)
#define ARC_LOG_FATAL(...)
#endif // DEBUG || RELEASE
