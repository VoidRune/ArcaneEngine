#include "ArcaneEngine/Graphics/VulkanCore/VulkanHandles.h"
#include "ArcaneEngine/Graphics/VulkanCore/VulkanLocal.h"
#include <vulkan/vulkan_core.h>
#include <vector>
#include <unordered_map>

namespace Arc
{
    VkDescriptorSetLayout GetDescriptorSetLayout(VkDevice device, std::unordered_map<uint64_t, DescriptorSetLayoutHandle>& map, const std::vector<VkDescriptorSetLayoutBinding>& bindings, uint32_t flags)
    {
        uint64_t key = std::hash<uint32_t>{}(flags);
        for (const auto& binding : bindings) {
            key ^= std::hash<uint64_t>{}(binding.binding) + 0x9e3779b9 + (key << 6) + (key >> 2);
            key ^= std::hash<uint64_t>{}(binding.descriptorType) + 0x9e3779b9 + (key << 6) + (key >> 2);
            key ^= std::hash<uint64_t>{}(binding.descriptorCount) + 0x9e3779b9 + (key << 6) + (key >> 2);
            key ^= std::hash<uint64_t>{}(binding.stageFlags) + 0x9e3779b9 + (key << 6) + (key >> 2);
        }

        if (map.contains(key))
        {
            return (VkDescriptorSetLayout)map[key];
        }

        std::vector<VkDescriptorBindingFlags> bindingFlags(bindings.size());
        for (auto& bf : bindingFlags)
        {
            bf = VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT;
        }
        
        VkDescriptorSetLayoutBindingFlagsCreateInfo bindingFlagsInfo{};
        bindingFlagsInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO;
        bindingFlagsInfo.bindingCount = (uint32_t)bindingFlags.size();
        bindingFlagsInfo.pBindingFlags = bindingFlags.data();

        VkDescriptorSetLayoutCreateInfo layoutInfo = {};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.pNext = &bindingFlagsInfo;
        layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
        layoutInfo.pBindings = bindings.data();
        layoutInfo.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT | flags;

        VkDescriptorSetLayout layout;
        VK_CHECK(vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &layout));

        map[key] = layout;

        return layout;
    }
}