#include "Log.h"
#include <iostream>
#include <fstream>
#include <chrono>

namespace Arc
{
	void GetTimeStamp(char* timestamp, int bufferSize)
	{
		time_t now = time(0);
		tm* timeinfo = localtime(&now);
		strftime(timestamp, bufferSize,
			"%d-%m-%Y %H:%M:%S", timeinfo);
	}

	void LogInfo(const std::string& msg)
	{
		char timestamp[20];
		GetTimeStamp(timestamp, sizeof(timestamp));
		std::cout << "[" << timestamp << "] INFO: " << msg << std::endl;
	}

	void LogWarning(const std::string& msg)
	{
		char timestamp[20];
		GetTimeStamp(timestamp, sizeof(timestamp));
		std::cout << "[" << timestamp << "] WARN: " << msg << std::endl;
	}

	void LogError(const std::string& msg)
	{
		char timestamp[20];
		GetTimeStamp(timestamp, sizeof(timestamp));
		std::cout << "[" << timestamp << "] ERROR: " << msg << std::endl;
	}

	void LogFatal(const std::string& msg)
	{
		char timestamp[20];
		GetTimeStamp(timestamp, sizeof(timestamp));
		std::cout << "[" << timestamp << "] FATAL: " << msg << std::endl;
	}
}