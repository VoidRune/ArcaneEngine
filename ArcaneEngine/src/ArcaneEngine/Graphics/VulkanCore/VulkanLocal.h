#pragma once
#include <iostream>

#define ARCANE_ENABLE_VALIDATION
#ifdef ARCANE_ENABLE_VALIDATION
#define VK_CHECK(x)															\
	{																		\
		int32_t err_temp = x;												\
		if (err_temp)														\
		{																	\
			std::cout <<"Detected Vulkan error: " << err_temp << std::endl; \
			abort();														\
		}																	\
	}
#else
#define VK_CHECK(x) x
#endif
