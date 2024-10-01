#include "ArcaneEngine/Graphics/ResourceCache.h"
#include "ArcaneEngine/Core/Log.h"
#include "ArcaneEngine/Graphics/VulkanCore/VulkanLocal.h"
#include <vulkan/vulkan_core.h>
#include <spirv_cross/spirv_cross.hpp>

namespace Arc
{
	void ResourceCache::CreateShader(Shader* shader, const ShaderDesc& desc)
	{
        if (desc.SpirV.size() == 0)
        {
            ARC_LOG_ERROR("SpirV size is 0");
            return;
        }

        VkShaderModuleCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        createInfo.codeSize = desc.SpirV.size() * sizeof(uint32_t);
        createInfo.pCode = reinterpret_cast<const uint32_t*>(desc.SpirV.data());

        VkShaderModule shaderModule;
        VK_CHECK(vkCreateShaderModule((VkDevice)m_LogicalDevice, &createInfo, nullptr, &shaderModule));
        shader->m_Module = shaderModule;
        shader->m_EntryPoint = desc.EntryPoint;
        shader->m_ShaderStage = desc.ShaderStage;

        spirv_cross::Compiler compiler(desc.SpirV);
        spirv_cross::ShaderResources resources = compiler.get_shader_resources();

        // TODO reflect vertex attribute/binding data
        /*
        std::vector<VkVertexInputBindingDescription> inputBinding{};
        std::vector<VkVertexInputAttributeDescription> inputAttributes{};
        uint32_t attributeLocation = 0;
        uint32_t attributeOffset = 0;
        for (const auto& input : resources.stage_inputs)
        {
            const auto& bufferType = comp.get_type(input.base_type_id);
            bufferType.basetype;
            uint32_t width = bufferType.width / 8;
            uint32_t vecSize = bufferType.vecsize;
            uint32_t binding = comp.get_decoration(input.id, spv::DecorationBinding);
            VkFormat format = SpirvHelper::GetFormat(bufferType.basetype, bufferType.width, bufferType.vecsize);
            VkVertexInputAttributeDescription attribute;
            attribute.binding = 0;
            attribute.location = attributeLocation;
            attribute.format = format;
            attribute.offset = attributeOffset;
            inputAttributes.push_back(attribute);

            attributeLocation++;
            attributeOffset += width * vecSize;
        }
        if (inputAttributes.size() > 0)
        {
            VkVertexInputBindingDescription binding;
            binding.binding = 0;
            binding.stride = attributeOffset;
            binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
            inputBinding.push_back(binding);
        }*/

        for (const auto& resource : resources.push_constant_buffers)
        {
            // Should be maximum one push constant
            const auto& bufferType = compiler.get_type(resource.base_type_id);
            uint32_t size = compiler.get_declared_struct_size(bufferType);
            shader->m_PushConstantSize = size;
        }

        
        shader->m_StageOutputs = 0;
        for (const auto& output : resources.stage_outputs)
            shader->m_StageOutputs++;
        
        for (const auto& resource : resources.sampled_images)
        {
            const auto& bufferType = compiler.get_type(resource.base_type_id);
            uint32_t set = compiler.get_decoration(resource.id, spv::DecorationDescriptorSet);
            uint32_t binding = compiler.get_decoration(resource.id, spv::DecorationBinding);
            const spirv_cross::SPIRType& type = compiler.get_type(resource.type_id);
            uint32_t count = 1;
            if (type.array.size() > 0)
                count = type.array[0];

            Shader::DescriptorLayoutBinding layoutBinding;
            layoutBinding.setIndex = set;
            layoutBinding.binding = binding;
            layoutBinding.descriptorCount = count;
            layoutBinding.descriptorType = DescriptorType::CombinedImageSampler;
            shader->m_LayoutBindings.push_back(layoutBinding);
        }

        for (const auto& resource : resources.uniform_buffers)
        {
            const auto& bufferType = compiler.get_type(resource.base_type_id);
            uint32_t bufferSize = compiler.get_declared_struct_size(bufferType);
            uint32_t set = compiler.get_decoration(resource.id, spv::DecorationDescriptorSet);
            uint32_t binding = compiler.get_decoration(resource.id, spv::DecorationBinding);
            const spirv_cross::SPIRType& type = compiler.get_type(resource.type_id);
            uint32_t count = 1;
            if (type.array.size() > 0)
                count = type.array[0];

            Shader::DescriptorLayoutBinding layoutBinding;
            layoutBinding.setIndex = set;
            layoutBinding.binding = binding;
            layoutBinding.descriptorCount = count;
            layoutBinding.descriptorType = DescriptorType::UniformBuffer;
            shader->m_LayoutBindings.push_back(layoutBinding);
        }

        for (const auto& resource : resources.storage_buffers)
        {
            const auto& bufferType = compiler.get_type(resource.base_type_id);
            uint32_t bufferSize = compiler.get_declared_struct_size(bufferType);
            uint32_t set = compiler.get_decoration(resource.id, spv::DecorationDescriptorSet);
            uint32_t binding = compiler.get_decoration(resource.id, spv::DecorationBinding);
            const spirv_cross::SPIRType& type = compiler.get_type(resource.type_id);
            uint32_t count = 1;
            if (type.array.size() > 0)
                count = type.array[0];

            Shader::DescriptorLayoutBinding layoutBinding;
            layoutBinding.setIndex = set;
            layoutBinding.binding = binding;
            layoutBinding.descriptorCount = count;
            layoutBinding.descriptorType = DescriptorType::StorageBuffer;
            shader->m_LayoutBindings.push_back(layoutBinding);
        }

        for (const auto& resource : resources.separate_images)
        {
            const auto& bufferType = compiler.get_type(resource.base_type_id);
            uint32_t set = compiler.get_decoration(resource.id, spv::DecorationDescriptorSet);
            uint32_t binding = compiler.get_decoration(resource.id, spv::DecorationBinding);
            const spirv_cross::SPIRType& type = compiler.get_type(resource.type_id);
            uint32_t count = 1;
            if (type.array.size() > 0)
                count = type.array[0];

            Shader::DescriptorLayoutBinding layoutBinding;
            layoutBinding.setIndex = set;
            layoutBinding.binding = binding;
            layoutBinding.descriptorCount = count;
            layoutBinding.descriptorType = DescriptorType::SampledImage;
            shader->m_LayoutBindings.push_back(layoutBinding);
        }

        for (const auto& resource : resources.storage_images)
        {
            const auto& bufferType = compiler.get_type(resource.base_type_id);
            uint32_t set = compiler.get_decoration(resource.id, spv::DecorationDescriptorSet);
            uint32_t binding = compiler.get_decoration(resource.id, spv::DecorationBinding);
            const spirv_cross::SPIRType& type = compiler.get_type(resource.type_id);
            uint32_t count = 1;
            if (type.array.size() > 0)
                count = type.array[0];

            Shader::DescriptorLayoutBinding layoutBinding;
            layoutBinding.setIndex = set;
            layoutBinding.binding = binding;
            layoutBinding.descriptorCount = count;
            layoutBinding.descriptorType = DescriptorType::StorageImage;
            shader->m_LayoutBindings.push_back(layoutBinding);
        }

        for (const auto& resource : resources.separate_samplers)
        {
            const auto& bufferType = compiler.get_type(resource.base_type_id);
            uint32_t set = compiler.get_decoration(resource.id, spv::DecorationDescriptorSet);
            uint32_t binding = compiler.get_decoration(resource.id, spv::DecorationBinding);
            const spirv_cross::SPIRType& type = compiler.get_type(resource.type_id);
            uint32_t count = 1;
            if (type.array.size() > 0)
                count = type.array[0];

            Shader::DescriptorLayoutBinding layoutBinding;
            layoutBinding.setIndex = set;
            layoutBinding.binding = binding;
            layoutBinding.descriptorCount = count;
            layoutBinding.descriptorType = DescriptorType::Sampler;
            shader->m_LayoutBindings.push_back(layoutBinding);
        }

        m_Resources[shader] = ResourceType::Shader;
	}

    void ResourceCache::ReleaseResource(Shader* shader)
    {
        if (!m_Resources.contains(shader))
        {
            ARC_LOG_FATAL("Cannot find Shader resource to release!");
        }
        vkDestroyShaderModule((VkDevice)m_LogicalDevice, (VkShaderModule)shader->m_Module, nullptr);
        m_Resources.erase(shader);
    }
}