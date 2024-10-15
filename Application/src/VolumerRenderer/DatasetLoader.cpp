#include "DatasetLoader.h"
#include "ArcaneEngine/Core/Log.h"
#include <fstream>

namespace DatasetLoader
{
	std::vector<uint8_t> LoadFromFile(std::string filePath)
	{
		std::vector<uint8_t> data;
		std::ifstream in(filePath, std::ios::in | std::ios::binary);
		if (in.is_open())
		{
			in.seekg(0, std::ios::end);
			data.resize(in.tellg());
			in.seekg(0, std::ios::beg);
			in.read((char*)data.data(), data.size());
			in.close();
		}
		else
		{
			ARC_LOG_WARNING(std::string("Could not load dataset: " + filePath));
		}
		return data;
	}
}