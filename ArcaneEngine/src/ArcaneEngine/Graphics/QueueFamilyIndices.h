#pragma once
#include <cstdint>

namespace Arc
{
	struct QueueFamilyIndices
	{
		uint32_t GraphicsIndex = uint32_t(-1);
		uint32_t PresentIndex = uint32_t(-1);
		//uint32_t TransferIndex = uint32_t(-1);
	};
}