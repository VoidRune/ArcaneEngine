#pragma once
#include <iostream>

namespace Arc 
{
	const char* GetVulkanResultString(int32_t vkResult);
}

#if defined(DEBUG) || defined(RELEASE)
	#define ARCANE_ENABLE_VALIDATION
#endif // DEBUG || RELEASE

#ifdef ARCANE_ENABLE_VALIDATION
#define VK_CHECK(x)															\
	{																		\
		int32_t err_temp = x;												\
		if (err_temp)														\
		{																	\
			std::cout <<"Detected Vulkan error: " << Arc::GetVulkanResultString(err_temp) << std::endl; \
			abort();														\
		}																	\
	}
#else
#define VK_CHECK(x) x
#endif