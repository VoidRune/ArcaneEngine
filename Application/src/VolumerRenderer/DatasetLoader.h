#pragma once
#include <vector>
#include <string>

namespace DatasetLoader
{
	std::vector<std::string> GetDirectoryFiles(std::string path);
	std::vector<uint8_t> LoadFromFile(std::string filePath);
}