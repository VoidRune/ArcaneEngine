#pragma once

#define WIN32_LEAN_AND_MEAN
#define NOCOMM
#define NOMINMAX
#define NOGDI
#include <vulkan/vulkan_core.h>

#include <iostream>

//#define ARCANE_ENABLE_VALIDATION
#ifdef ARCANE_ENABLE_VALIDATION
#define VK_CHECK(x)															\
	{																		\
		VkResult err_temp = x;												\
		if (err_temp)														\
		{																	\
			std::cout <<"Detected Vulkan error: " << err_temp << std::endl; \
			abort();														\
		}																	\
	}
#else
#define VK_CHECK(x) x
#endif

#include <vector>
#include <map>
#include <array>
#include <string>
#include <memory>
#include <functional>

typedef struct HINSTANCE__* HINSTANCE;
typedef struct HWND__* HWND;
typedef struct HMONITOR__* HMONITOR;
typedef void* HANDLE;
typedef /*_Null_terminated_*/ const wchar_t* LPCWSTR;
typedef unsigned long DWORD;
typedef struct _SECURITY_ATTRIBUTES SECURITY_ATTRIBUTES;

namespace Arc
{
	struct QueueFamilyIndices
	{
		uint32_t graphicsFamilyIndex;
		uint32_t presentFamilyIndex;
	};
}