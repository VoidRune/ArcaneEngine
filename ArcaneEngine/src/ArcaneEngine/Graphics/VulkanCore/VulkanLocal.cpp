#include "VulkanLocal.h"
#include <vulkan/vk_enum_string_helper.h>

namespace Arc
{
	const char* GetVulkanResultString(int32_t vkResult)
	{
		return string_VkResult((VkResult)vkResult);
	}
}