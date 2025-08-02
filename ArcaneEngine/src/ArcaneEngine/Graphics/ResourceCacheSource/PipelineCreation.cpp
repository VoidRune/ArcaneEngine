#pragma once
#include "ArcaneEngine/Graphics/ResourceCache.h"
#include "ArcaneEngine/Core/Log.h"
#include "ArcaneEngine/Graphics/VulkanCore/VulkanLocal.h"
#include <vulkan/vulkan_core.h>
#include <map>

namespace Arc
{
    extern VkDescriptorSetLayout GetDescriptorSetLayout(VkDevice device, std::unordered_map<uint64_t, DescriptorSetLayoutHandle>& map, const std::vector<VkDescriptorSetLayoutBinding>& bindings, uint32_t flags);

	void ResourceCache::CreatePipeline(Pipeline* pipeline, const PipelineDesc& desc)
	{
        std::vector<VkDynamicState> dynamicStates = {
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_SCISSOR
        };

        //if (desc.DepthBiasEnabled)
        //    dynamicStates.push_back(VK_DYNAMIC_STATE_DEPTH_BIAS);

        VkPipelineDynamicStateCreateInfo dynamicState = {};
        dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
        dynamicState.pDynamicStates = dynamicStates.data();


        std::vector<VkVertexInputAttributeDescription> vertexAttributes = {};
        std::vector<VkVertexInputBindingDescription> vertexBinding = {};
        for (int i = 0; i < desc.VertexAttributes.m_Attributes.size(); i++)
        {
            VkVertexInputAttributeDescription attribute;
            attribute.binding = 0;
            attribute.location = i;
            attribute.format = static_cast<VkFormat>(desc.VertexAttributes.m_Attributes[i].format);
            attribute.offset = desc.VertexAttributes.m_Attributes[i].offset;
            vertexAttributes.push_back(attribute);
        }
        if (desc.VertexAttributes.m_VertexSize > 0)
        {
            VkVertexInputBindingDescription binding;
            binding.binding = 0;
            binding.stride = desc.VertexAttributes.m_VertexSize;
            binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
            vertexBinding.push_back(binding);
        }

        VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
        vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(vertexAttributes.size());
        vertexInputInfo.pVertexAttributeDescriptions = vertexAttributes.data();
        vertexInputInfo.vertexBindingDescriptionCount = static_cast<uint32_t>(vertexBinding.size());
        vertexInputInfo.pVertexBindingDescriptions = vertexBinding.data();

        VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo = {};
        inputAssemblyInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssemblyInfo.topology = static_cast<VkPrimitiveTopology>(desc.Topology);
        inputAssemblyInfo.primitiveRestartEnable = VK_FALSE;

        VkPipelineViewportStateCreateInfo viewportInfo = {};
        viewportInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportInfo.viewportCount = 1;
        viewportInfo.pViewports = nullptr;
        viewportInfo.scissorCount = 1;
        viewportInfo.pScissors = nullptr;

        VkPipelineRasterizationStateCreateInfo rasterizationInfo = {};
        rasterizationInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizationInfo.depthClampEnable = VK_FALSE;
        rasterizationInfo.rasterizerDiscardEnable = VK_FALSE;
        rasterizationInfo.polygonMode = VK_POLYGON_MODE_FILL;
        rasterizationInfo.lineWidth = 1.0f;
        rasterizationInfo.cullMode = static_cast<VkCullModeFlags>(desc.CullMode);
        rasterizationInfo.frontFace = VK_FRONT_FACE_CLOCKWISE;
        rasterizationInfo.depthBiasEnable = VK_FALSE;
        rasterizationInfo.depthBiasConstantFactor = 0.0f;
        rasterizationInfo.depthBiasClamp = 0.0f;
        rasterizationInfo.depthBiasSlopeFactor = 0.0f;

        VkPipelineMultisampleStateCreateInfo multisampleInfo = {};
        multisampleInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampleInfo.sampleShadingEnable = VK_FALSE;
        multisampleInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
        multisampleInfo.minSampleShading = 1.0f;
        multisampleInfo.pSampleMask = nullptr;
        multisampleInfo.alphaToCoverageEnable = VK_FALSE;
        multisampleInfo.alphaToOneEnable = VK_FALSE;

        VkPipelineDepthStencilStateCreateInfo depthStencilInfo = {};
        depthStencilInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        depthStencilInfo.depthTestEnable = (desc.DepthTest == true) ? VK_TRUE : VK_FALSE;
        depthStencilInfo.depthWriteEnable = (desc.DepthWrite == true) ? VK_TRUE : VK_FALSE;
        depthStencilInfo.depthCompareOp = static_cast<VkCompareOp>(desc.DepthCompareOp);
        depthStencilInfo.depthBoundsTestEnable = VK_FALSE;
        depthStencilInfo.minDepthBounds = 0.0f;
        depthStencilInfo.maxDepthBounds = 1.0f;
        depthStencilInfo.stencilTestEnable = VK_FALSE;
        depthStencilInfo.front = {};
        depthStencilInfo.back = {};

        uint32_t colorAttachmentCount = 0;
        uint32_t pushConstantSize = 0;
        for (const auto& shader : desc.ShaderStages)
        {
            if (shader->m_ShaderStage == ShaderStage::Fragment)
                colorAttachmentCount = shader->m_StageOutputs;
            if (shader->m_PushConstantSize > pushConstantSize)
                pushConstantSize = shader->m_PushConstantSize;
        }

        std::vector<VkPipelineColorBlendAttachmentState> colorBlendAttachment;
        for (uint32_t i = 0; i < colorAttachmentCount; i++)
        {
            VkPipelineColorBlendAttachmentState tempAttachment;
            tempAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
            tempAttachment.blendEnable = VK_FALSE;
            tempAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
            tempAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
            tempAttachment.colorBlendOp = VK_BLEND_OP_ADD;
            tempAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
            tempAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
            tempAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
            colorBlendAttachment.push_back(tempAttachment);
        }

        VkPipelineColorBlendStateCreateInfo colorBlendInfo = {};
        colorBlendInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlendInfo.logicOpEnable = VK_FALSE;
        colorBlendInfo.logicOp = VK_LOGIC_OP_COPY;
        colorBlendInfo.attachmentCount = static_cast<uint32_t>(colorBlendAttachment.size());
        colorBlendInfo.pAttachments = colorBlendAttachment.data();
        colorBlendInfo.blendConstants[0] = 0.0f;
        colorBlendInfo.blendConstants[1] = 0.0f;
        colorBlendInfo.blendConstants[2] = 0.0f;
        colorBlendInfo.blendConstants[3] = 0.0f;

        //if (colorAttachmentCount != desc.ColorAttachmentFormats.size())
        //{
        //    ARC_LOG_WARNING(std::string("Pipeline creation, ColorAttachmentFormats size(") + std::to_string(desc.ColorAttachmentFormats.size()) + ") different from shader color attachments(" + std::to_string(colorAttachmentCount) + ")!");
        //}

        std::vector<VkFormat> colorAttachmentFormats(desc.ColorAttachmentFormats.size());
        for (size_t i = 0; i < colorAttachmentFormats.size(); i++)
            colorAttachmentFormats[i] = (VkFormat)desc.ColorAttachmentFormats[i];

        VkPipelineRenderingCreateInfo renderingInfo = {};
        renderingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
        renderingInfo.colorAttachmentCount = static_cast<uint32_t>(colorAttachmentFormats.size());
        renderingInfo.pColorAttachmentFormats = colorAttachmentFormats.data();
        renderingInfo.depthAttachmentFormat = (VkFormat)desc.DepthAttachmentFormat;
        renderingInfo.stencilAttachmentFormat = VK_FORMAT_UNDEFINED;

        VkPushConstantRange pushConstantRange{};
        pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
        pushConstantRange.offset = 0;
        pushConstantRange.size = pushConstantSize;

        std::map<uint32_t, std::map<uint32_t, VkDescriptorSetLayoutBinding>> bindings;
        for (const auto& shader : desc.ShaderStages)
        {
            for (auto& b : shader->m_LayoutBindings)
            {
                auto& set = bindings[b.setIndex];
                if (!set.contains(b.binding))
                {
                    VkDescriptorSetLayoutBinding binding = {};
                    binding.binding = b.binding;
                    binding.descriptorType = (VkDescriptorType)b.descriptorType;
                    binding.descriptorCount = b.descriptorCount;
                    binding.stageFlags = (VkShaderStageFlags)shader->m_ShaderStage;
                    set[b.binding] = binding;
                }
                else
                {
                    set[b.binding].stageFlags |= (VkShaderStageFlagBits)shader->m_ShaderStage;
                }
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


        VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(layouts.size());
        pipelineLayoutInfo.pSetLayouts = layouts.data();
        if (pushConstantSize > 0)
        {
            pipelineLayoutInfo.pushConstantRangeCount = 1;
            pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;
        }

        VkPipelineLayout pipelineLayout;
        VK_CHECK(vkCreatePipelineLayout((VkDevice)m_LogicalDevice, &pipelineLayoutInfo, nullptr, &pipelineLayout));
        pipeline->m_PipelineLayout = pipelineLayout;

        std::vector<VkPipelineShaderStageCreateInfo> shaderStages(desc.ShaderStages.size());
        for (int i = 0; i < shaderStages.size(); i++)
        {
            shaderStages[i] = {};
            shaderStages[i].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            shaderStages[i].stage = (VkShaderStageFlagBits)desc.ShaderStages[i]->m_ShaderStage;
            shaderStages[i].module = (VkShaderModule)desc.ShaderStages[i]->m_Module;
            shaderStages[i].pName = desc.ShaderStages[i]->m_EntryPoint.c_str();
            shaderStages[i].flags = 0;
            shaderStages[i].pNext = nullptr;
            shaderStages[i].pSpecializationInfo = nullptr;
        }

        VkGraphicsPipelineCreateInfo pipelineInfo = {};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineInfo.pNext = &renderingInfo;
        pipelineInfo.stageCount = static_cast<uint32_t>(shaderStages.size());
        pipelineInfo.pStages = shaderStages.data();

        pipelineInfo.pVertexInputState = &vertexInputInfo;
        pipelineInfo.pInputAssemblyState = &inputAssemblyInfo;
        pipelineInfo.pViewportState = &viewportInfo;
        pipelineInfo.pRasterizationState = &rasterizationInfo;
        pipelineInfo.pMultisampleState = &multisampleInfo;
        pipelineInfo.pDepthStencilState = &depthStencilInfo;
        pipelineInfo.pColorBlendState = &colorBlendInfo;
        pipelineInfo.pDynamicState = &dynamicState;

        pipelineInfo.layout = (VkPipelineLayout)pipeline->m_PipelineLayout;
        pipelineInfo.renderPass = VK_NULL_HANDLE;
        pipelineInfo.subpass = 0;// info.subpassIndex;

        pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
        pipelineInfo.basePipelineIndex = -1;

        VkPipeline pipelineTemp;
        VK_CHECK(vkCreateGraphicsPipelines((VkDevice)m_LogicalDevice, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipelineTemp));
        pipeline->m_Pipeline = pipelineTemp;

        m_Resources[pipeline] = ResourceType::Pipeline;
	}

    void ResourceCache::ReleaseResource(Pipeline* pipeline)
    {
        if (!m_Resources.contains(pipeline))
        {
            ARC_LOG_FATAL("Cannot find Pipeline resource to release!");
        }
        vkDestroyPipelineLayout((VkDevice)m_LogicalDevice, (VkPipelineLayout)pipeline->m_PipelineLayout, nullptr);
        vkDestroyPipeline((VkDevice)m_LogicalDevice, (VkPipeline)pipeline->m_Pipeline, nullptr);
        m_Resources.erase(pipeline);
    }
}