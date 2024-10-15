#pragma once
#include <vector>
#include <string>

namespace DatasetLoader
{
	std::vector<uint8_t> LoadFromFile(std::string filePath);
}