#include "ArcaneEngine/Graphics/ResourceCache.h"
#include "ArcaneEngine/Core/Log.h"
#include "ArcaneEngine/Graphics/VulkanCore/VulkanLocal.h"
#include <vulkan/vulkan_core.h>
#include <map>

namespace Arc
{
    extern VkDescriptorSetLayout GetDescriptorSetLayout(VkDevice device, std::unordered_map<uint64_t, DescriptorSetLayoutHandle>& map, const std::vector<VkDescriptorSetLayoutBinding>& bindings, uint32_t flags);

	void ResourceCache::CreateComputePipeline(ComputePipeline* pipeline, const ComputePipelineDesc& desc)
	{
        std::map<uint32_t, std::map<uint32_t, VkDescriptorSetLayoutBinding>> bindings;
        for (auto& b : desc.Shader->m_LayoutBindings)
        {
            auto& set = bindings[b.setIndex];
            if (!set.contains(b.binding))
            {
                VkDescriptorSetLayoutBinding binding = {};
                binding.binding = b.binding;
                binding.descriptorType = (VkDescriptorType)b.descriptorType;
                binding.descriptorCount = b.descriptorCount;
                binding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
                set[b.binding] = binding;
            }
        }

        std::vector<VkDescriptorSetLayout> layouts;
        for (auto& set : bindings)
        {
            uint32_t flags = desc.UsePushDescriptors ? VK_DESCRIPTOR_SET_LAYOUT_CREATE_PUSH_DESCRIPTOR_BIT : 0;
            std::vector<VkDescriptorSetLayoutBinding> layoutBindings;
            for (auto& binding : set.second)
            {
                layoutBindings.push_back(binding.second);
                //if (binding.second.descriptorCount >= MAX_BINDLESS_DESCRIPTOR_COUNT)
                //    flags |= (uint32_t)DescriptorFlags::Bindless;
            }
            layouts.push_back(GetDescriptorSetLayout((VkDevice)m_LogicalDevice, m_DescriptorSetLayouts, layoutBindings, flags));
        }

        VkPushConstantRange pushConstantRange = {};
        pushConstantRange.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
        pushConstantRange.offset = 0;
        pushConstantRange.size = desc.Shader->m_PushConstantSize;

        VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(layouts.size());
        pipelineLayoutInfo.pSetLayouts = layouts.data();
        if (desc.Shader->m_PushConstantSize > 0)
        {
            pipelineLayoutInfo.pushConstantRangeCount = 1;
            pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;
        }

        VkPipelineLayout pipelineLayout;
        VK_CHECK(vkCreatePipelineLayout((VkDevice)m_LogicalDevice, &pipelineLayoutInfo, nullptr, &pipelineLayout));

        VkPipelineShaderStageCreateInfo shaderStage = {};
        shaderStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        shaderStage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
        shaderStage.module = (VkShaderModule)desc.Shader->m_Module;
        shaderStage.pName = desc.Shader->m_EntryPoint.c_str();
        shaderStage.flags = 0;
        shaderStage.pNext = nullptr;
        shaderStage.pSpecializationInfo = nullptr;

        VkComputePipelineCreateInfo pipelineInfo{};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
        pipelineInfo.layout = pipelineLayout;
        pipelineInfo.stage = shaderStage;

        VkPipeline computePipeline;
        VK_CHECK(vkCreateComputePipelines((VkDevice)m_LogicalDevice, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &computePipeline));
        pipeline->m_Pipeline = computePipeline;
        pipeline->m_PipelineLayout = pipelineLayout;

        m_Resources[pipeline] = ResourceType::ComputePipeline;
	}

    void ResourceCache::ReleaseResource(ComputePipeline* computePipeline)
    {
        if (!m_Resources.contains(computePipeline))
        {
            ARC_LOG_FATAL("Cannot find ComputePipeline resource to release!");
        }
        vkDestroyPipelineLayout((VkDevice)m_LogicalDevice, (VkPipelineLayout)computePipeline->m_PipelineLayout, nullptr);
        vkDestroyPipeline((VkDevice)m_LogicalDevice, (VkPipeline)computePipeline->m_Pipeline, nullptr);
        m_Resources.erase(computePipeline);
    }
}