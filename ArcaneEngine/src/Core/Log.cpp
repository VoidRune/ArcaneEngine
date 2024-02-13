#include "Log.h"
#include <iostream>
#include <fstream>
#include <chrono>

namespace Arc
{
	std::ofstream file;
	bool logToConsole = true;

	void SetLogToFile(const std::string& fileName)
	{
		file.close();
		if(fileName.length() != 0)
			file.open(fileName);
	}

	void SetLogToConsole(bool setLogToConsole)
	{
		logToConsole = setLogToConsole;
	}

	void LogInfo(const std::string& msg)
	{
		std::time_t now_c = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
		struct tm* parts = std::localtime(&now_c);
		std::string date = "[" + std::to_string(parts->tm_mday) + "." 
			+ std::to_string(1 + parts->tm_mon) + "."
			+ std::to_string(1900 + parts->tm_year) + "-"
			+ std::to_string(parts->tm_hour) + ":"
			+ std::to_string(parts->tm_min) + ":"
			+ std::to_string(parts->tm_sec) + "] ";

		file << date << msg << std::endl;
		if (logToConsole) std::cout << date << msg << std::endl;
	}

	void LogWarning(const std::string& msg)
	{
		std::time_t now_c = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
		struct tm* parts = std::localtime(&now_c);
		std::string date = "[" + std::to_string(parts->tm_mday) + "."
			+ std::to_string(1 + parts->tm_mon) + "."
			+ std::to_string(1900 + parts->tm_year) + "-"
			+ std::to_string(parts->tm_hour) + ":"
			+ std::to_string(parts->tm_min) + ":"
			+ std::to_string(parts->tm_sec) + "] ";

		file << date << "WARNING: " << msg << std::endl;
		if (logToConsole) std::cout << date << "WARNING: " << msg << std::endl;
	}

	void LogError(const std::string& msg)
	{
		std::time_t now_c = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
		struct tm* parts = std::localtime(&now_c);
		std::string date = "[" + std::to_string(parts->tm_mday) + "."
			+ std::to_string(1 + parts->tm_mon) + "."
			+ std::to_string(1900 + parts->tm_year) + "-"
			+ std::to_string(parts->tm_hour) + ":"
			+ std::to_string(parts->tm_min) + ":"
			+ std::to_string(parts->tm_sec) + "] ";

		file << date << "ERROR: " << msg << std::endl;
		if (logToConsole) std::cout << date << "ERROR: " << msg << std::endl;
	}
}