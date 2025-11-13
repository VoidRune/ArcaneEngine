#include "ArcaneEngine/Graphics/ResourceCache.h"
#include "ArcaneEngine/Core/Log.h"
#include "ArcaneEngine/Graphics/VulkanCore/VulkanLocal.h"
#include <vulkan/vulkan_core.h>

namespace Arc
{
    extern VkDescriptorSetLayout GetDescriptorSetLayout(VkDevice device, std::unordered_map<uint64_t, DescriptorSetLayoutHandle>& map, const std::vector<VkDescriptorSetLayoutBinding>& bindings, uint32_t flags);

	void ResourceCache::AllocateDescriptorSet(DescriptorSet* descriptor, const DescriptorSetDesc& desc)
	{
        std::vector<VkDescriptorSetLayoutBinding> bindings;
        std::vector<DescriptorType> types;
        for (int i = 0; i < desc.Bindings.size(); i++)
        {
            VkDescriptorSetLayoutBinding binding = {};
            binding.binding = i;
            binding.descriptorCount = desc.Bindings[i].Flags == DescriptorFlag::Bindless ? 1024 : 1;
            binding.descriptorType = (VkDescriptorType)desc.Bindings[i].Type;
            binding.stageFlags = (VkShaderStageFlags)desc.Bindings[i].ShaderStage;
            binding.pImmutableSamplers = nullptr;
            bindings.push_back(binding);

            types.push_back(desc.Bindings[i].Type);
        }

        VkDescriptorSetLayout layout = GetDescriptorSetLayout((VkDevice)m_LogicalDevice, m_DescriptorSetLayouts, bindings, 0);

        VkDescriptorSetAllocateInfo descAllocInfo = {};
        descAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        descAllocInfo.descriptorPool = (VkDescriptorPool)m_DescriptorPool;
        descAllocInfo.descriptorSetCount = 1;
        descAllocInfo.pSetLayouts = &layout;

        VkDescriptorSet descriptorSet;
        VK_CHECK(vkAllocateDescriptorSets((VkDevice)m_LogicalDevice, &descAllocInfo, &descriptorSet));
        descriptor->m_DescriptorSet = descriptorSet;
        descriptor->m_BindingTypes = types;
	}

    void ResourceCache::AllocateDescriptorSetArray(DescriptorSetArray* descriptorArray, const DescriptorSetDesc& desc)
    {
        std::vector<VkDescriptorSetLayoutBinding> bindings;
        std::vector<DescriptorType> types;
        for (int i = 0; i < desc.Bindings.size(); i++)
        {
            VkDescriptorSetLayoutBinding binding = {};
            binding.binding = i;
            binding.descriptorCount = 1;
            binding.descriptorType = (VkDescriptorType)desc.Bindings[i].Type;
            binding.stageFlags = (VkShaderStageFlags)desc.Bindings[i].ShaderStage;
            binding.pImmutableSamplers = nullptr;
            bindings.push_back(binding);

            types.push_back(desc.Bindings[i].Type);
        }

        VkDescriptorSetLayout layout = GetDescriptorSetLayout((VkDevice)m_LogicalDevice, m_DescriptorSetLayouts, bindings, 0);

        std::vector<VkDescriptorSetLayout> layouts(m_FramesInFlight);
        for (int i = 0; i < layouts.size(); i++)
            layouts[i] = layout;

        descriptorArray->m_DescriptorSets.resize(m_FramesInFlight);
        descriptorArray->m_BindingTypes = types;

        VkDescriptorSetAllocateInfo descAllocInfo = {};
        descAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        descAllocInfo.descriptorPool = (VkDescriptorPool)m_DescriptorPool;
        descAllocInfo.descriptorSetCount = m_FramesInFlight;
        descAllocInfo.pSetLayouts = layouts.data();

        std::vector<VkDescriptorSet> descriptorSets(m_FramesInFlight);
        VK_CHECK(vkAllocateDescriptorSets((VkDevice)m_LogicalDevice, &descAllocInfo, descriptorSets.data()));

        for (uint32_t i = 0; i < m_FramesInFlight; i++)
        {
            descriptorArray->m_DescriptorSets[i] = descriptorSets[i];
        }
    }
}