#pragma once
#include <vector>

namespace Decoder
{

	void OggFromFile(const char* filename, std::vector<int16_t>& data, uint32_t& sampleRate, uint32_t& sizeInFrames, uint32_t& channels);
	void Mp3FromFile(const char* filename, std::vector<int16_t>& data, uint32_t& sampleRate, uint32_t& sizeInFrames, uint32_t& channels);

}