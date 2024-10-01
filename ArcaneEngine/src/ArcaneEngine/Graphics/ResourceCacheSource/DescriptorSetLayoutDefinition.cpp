#include "ArcaneEngine/Graphics/VulkanCore/VulkanHandles.h"
#include "ArcaneEngine/Graphics/VulkanCore/VulkanLocal.h"
#include <vulkan/vulkan_core.h>
#include <vector>
#include <unordered_map>

namespace Arc
{
    VkDescriptorSetLayout GetDescriptorSetLayout(VkDevice device, std::unordered_map<uint64_t, DescriptorSetLayoutHandle>& map, const std::vector<VkDescriptorSetLayoutBinding>& bindings, uint32_t flags)
    {
        uint64_t key = flags;
        for (const auto& binding : bindings) {
            key ^= static_cast<uint64_t>(binding.binding) << 32;
            key ^= static_cast<uint64_t>(binding.descriptorType) << 24;
            key ^= static_cast<uint64_t>(binding.descriptorCount) << 16;
            key ^= static_cast<uint64_t>(binding.stageFlags) << 8;
        }

        if (map.contains(key))
        {
            return (VkDescriptorSetLayout)map[key];
        }

        VkDescriptorSetLayoutCreateInfo layoutInfo = {};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
        layoutInfo.pBindings = bindings.data();
        layoutInfo.flags = flags;

        VkDescriptorSetLayout layout;
        VK_CHECK(vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &layout));

        map[key] = layout;

        return layout;
    }
}