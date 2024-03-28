#include "ResourceCache.h"
#include "Core/Log.h"
#include <filesystem>
#include <map>

#include "spirv_cross/spirv_cross.hpp"
#ifdef _DEBUG
#pragma comment(lib, "spirv-cross-cored.lib")
#else
#pragma comment(lib, "spirv-cross-core.lib")
#endif

#define MAX_BINDLESS_DESCRIPTOR_COUNT 1024

namespace Arc
{
	ResourceCache::ResourceCache(VkInstance instance, VkDevice device, VkPhysicalDevice physicalDevice, uint32_t imageCount)
	{
		m_Device = device;
		m_PhysicalDevice = physicalDevice;
        m_ImageCount = imageCount;

        std::vector<VkDescriptorPoolSize> poolSizes =
        {
            {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1024},
            {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1024},
            {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1024},
            {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1024},
        };

        VkDescriptorPoolCreateInfo descriptorPoolInfo{};
        descriptorPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        descriptorPoolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
        descriptorPoolInfo.pPoolSizes = poolSizes.data();
        descriptorPoolInfo.maxSets = 1024;
        descriptorPoolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT_EXT;
        VK_CHECK(vkCreateDescriptorPool(m_Device, &descriptorPoolInfo, nullptr, &m_DescriptorPool));

        VmaAllocatorCreateInfo allocatorInfo{};
        allocatorInfo.physicalDevice = m_PhysicalDevice;
        allocatorInfo.device = m_Device;
        allocatorInfo.instance = instance;
        VK_CHECK(vmaCreateAllocator(&allocatorInfo, &m_MemoryAllocator));
	}

	ResourceCache::~ResourceCache()
	{
        FreeResources();
        vmaDestroyAllocator(m_MemoryAllocator);
        vkDestroyDescriptorPool(m_Device, m_DescriptorPool, nullptr);
	}

    void ResourceCache::CreateBuffer(GpuBuffer* buffer, const GpuBufferDesc& bufferDescription)
    {
        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = bufferDescription.Size;
        bufferInfo.usage = bufferDescription.UsageFlags;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        VmaAllocationCreateInfo allocInfo = {};
        allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
        allocInfo.requiredFlags = bufferDescription.MemoryVisibility;
        if (bufferDescription.MemoryVisibility & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT ||
            bufferDescription.MemoryVisibility & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT ||
            bufferDescription.MemoryVisibility & VK_MEMORY_PROPERTY_HOST_CACHED_BIT)
        {
            allocInfo.flags |= VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
        }

        VK_CHECK(vmaCreateBuffer(m_MemoryAllocator, &bufferInfo, &allocInfo,
            &buffer->m_Buffer,
            &buffer->m_Allocation,
            nullptr));
        
        buffer->m_Size = bufferDescription.Size;

        VkBuffer pBuffer = buffer->m_Buffer;
        VmaAllocation pBufferMemory = buffer->m_Allocation;

        m_ResourceReleaseFuctions[pBuffer] = [=]() {
            vmaDestroyBuffer(m_MemoryAllocator, pBuffer, pBufferMemory);
        };
    }

    void ResourceCache::CreateBufferSet(GpuBufferSet* buffer, const GpuBufferDesc& bufferDescription)
    {
        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = bufferDescription.Size;
        bufferInfo.usage = bufferDescription.UsageFlags;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        VmaAllocationCreateInfo allocInfo = {};
        allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
        allocInfo.requiredFlags = bufferDescription.MemoryVisibility;
        if (bufferDescription.MemoryVisibility & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT ||
            bufferDescription.MemoryVisibility & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT ||
            bufferDescription.MemoryVisibility & VK_MEMORY_PROPERTY_HOST_CACHED_BIT)
        {
            allocInfo.flags |= VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
        }

        buffer->m_Buffer.resize(m_ImageCount);
        buffer->m_Allocation.resize(m_ImageCount);

        for (uint32_t i = 0; i < m_ImageCount; i++)
        {
            VK_CHECK(vmaCreateBuffer(m_MemoryAllocator, &bufferInfo, &allocInfo,
                &buffer->m_Buffer[i],
                &buffer->m_Allocation[i],
                nullptr));
        }

        buffer->m_Size = bufferDescription.Size;

        std::vector<VkBuffer> pBuffer = buffer->m_Buffer;
        std::vector<VmaAllocation> pBufferMemory = buffer->m_Allocation;
        m_ResourceReleaseFuctions[pBuffer[0]] = [=]() {
            for (uint32_t i = 0; i < m_ImageCount; i++)
                vmaDestroyBuffer(m_MemoryAllocator, pBuffer[i], pBufferMemory[i]);
        };
    }

	void ResourceCache::CreateImage(Image* image, const ImageDesc& imageDescription)
	{
        image->m_Width = imageDescription.Extent.width;
        image->m_Height = imageDescription.Extent.height;
        image->m_Depth = imageDescription.Depth;
        image->m_Format = static_cast<VkFormat>(imageDescription.Format);

        image->m_MipLevels = 1;
        if(imageDescription.MipLevelsEnabled)
            image->m_MipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(image->m_Width, image->m_Height)))) + 1;

        VkImageType imageType = VK_IMAGE_TYPE_2D;
        VkImageViewType imageViewType = VK_IMAGE_VIEW_TYPE_2D;
        if (imageDescription.Depth > 1)
        {
            imageType = VK_IMAGE_TYPE_3D;
            imageViewType = VK_IMAGE_VIEW_TYPE_3D;
        }

        VkImageCreateInfo imageInfo = {};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.pNext = nullptr;
        imageInfo.imageType = imageType;
        imageInfo.format = static_cast<VkFormat>(imageDescription.Format);
        imageInfo.extent.width = imageDescription.Extent.width;
        imageInfo.extent.height = imageDescription.Extent.height;
        imageInfo.extent.depth = imageDescription.Depth;
        imageInfo.mipLevels = image->m_MipLevels;
        imageInfo.arrayLayers = 1;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageInfo.usage = imageDescription.UsageFlags;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.flags = 0; // Optional

        if (image->m_MipLevels > 1)
            imageInfo.usage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;

        VmaAllocationCreateInfo allocInfo = {};
        allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
        allocInfo.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        VK_CHECK(vmaCreateImage(m_MemoryAllocator, &imageInfo, &allocInfo, &image->m_Image, &image->m_Allocation, nullptr));

        VkImageViewCreateInfo imageViewInfo{};
        imageViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        imageViewInfo.image = image->m_Image;
        imageViewInfo.viewType = imageViewType;
        imageViewInfo.format = static_cast<VkFormat>(imageDescription.Format);
        imageViewInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewInfo.subresourceRange.aspectMask = imageDescription.AspectFlags;
        imageViewInfo.subresourceRange.baseMipLevel = 0;
        imageViewInfo.subresourceRange.levelCount = image->m_MipLevels;
        imageViewInfo.subresourceRange.baseArrayLayer = 0;
        imageViewInfo.subresourceRange.layerCount = 1;

        VK_CHECK(vkCreateImageView(m_Device, &imageViewInfo, nullptr, &image->m_ImageView));

        VkImage pImage = image->m_Image;
        VmaAllocation pImageMemory = image->m_Allocation;
        VkImageView pImageView = image->m_ImageView;
        m_ResourceReleaseFuctions[pImage] = [=]() {
            vmaDestroyImage(m_MemoryAllocator, pImage, pImageMemory);
            vkDestroyImageView(m_Device, pImageView, nullptr);
        };
	}

    void ResourceCache::CreateSampler(Sampler* sampler, const SamplerDesc& samplerDescription)
    {

        VkSamplerCreateInfo samplerInfo{};
        samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerInfo.magFilter = static_cast<VkFilter>(samplerDescription.MagFilter);
        samplerInfo.minFilter = static_cast<VkFilter>(samplerDescription.MinFilter);
        samplerInfo.addressModeU = static_cast<VkSamplerAddressMode>(samplerDescription.AddressMode);
        samplerInfo.addressModeV = samplerInfo.addressModeU;
        samplerInfo.addressModeW = samplerInfo.addressModeU;
        samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        samplerInfo.mipLodBias = 0.0f;
        samplerInfo.minLod = 0.0f;
        samplerInfo.maxLod = VK_LOD_CLAMP_NONE;
        samplerInfo.anisotropyEnable = VK_FALSE;
        samplerInfo.maxAnisotropy = 1.0f;
        samplerInfo.unnormalizedCoordinates = VK_FALSE;
        samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
        // Do not change compareEnable, it breaks the shadow mapping
        samplerInfo.compareEnable = VK_FALSE;
        samplerInfo.compareOp = VK_COMPARE_OP_NEVER;

        VK_CHECK(vkCreateSampler(m_Device, &samplerInfo, nullptr, &sampler->m_Sampler));

        VkSampler pSampler = sampler->m_Sampler;
        m_ResourceReleaseFuctions[pSampler] = [=]() {
            vkDestroySampler(m_Device, pSampler, nullptr);
        };

    }

    void ResourceCache::AllocateDescriptorSets(const std::vector<DescriptorSet*> descriptorSet, const DescriptorSetLayoutDesc& layoutDescription)
    {

        uint32_t inFlightCount = 1;
        if (layoutDescription.Flags & (uint32_t)DescriptorFlags::InFlight)
        {
            inFlightCount = m_ImageCount;
            for (size_t i = 0; i < descriptorSet.size(); i++)
            {
                descriptorSet[i]->m_InFlight = 0xFFFF;
            }
        }

        VkDescriptorSetLayout layout = GetDescriptorSetLayout(layoutDescription.LayoutBindings, layoutDescription.Flags);
        
        std::vector<VkDescriptorSetLayout> layoutArray(inFlightCount * descriptorSet.size());
        for (size_t i = 0; i < descriptorSet.size(); i++)
        {
            for (size_t j = 0; j < inFlightCount; j++)
            {
                layoutArray[i * inFlightCount + j] = layout;
            }
            for (auto& binding : layoutDescription.LayoutBindings)
            {
                descriptorSet[i]->m_BindingTypes.push_back(binding.descriptorType);
            }
        }

        VkDescriptorSetAllocateInfo descAllocInfo{};
        descAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        descAllocInfo.descriptorPool = m_DescriptorPool;
        descAllocInfo.descriptorSetCount = inFlightCount * descriptorSet.size();
        descAllocInfo.pSetLayouts = layoutArray.data();

        VkDescriptorSetVariableDescriptorCountAllocateInfoEXT countInfo{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_ALLOCATE_INFO_EXT };
        uint32_t max_binding = 1024 - 1;
        if (layoutDescription.Flags & (uint32_t)DescriptorFlags::Bindless)
        {
            countInfo.descriptorSetCount = 1;
            // This number is the max allocatable count
            countInfo.pDescriptorCounts = &max_binding;
            descAllocInfo.pNext = &countInfo;
        }

        std::vector<VkDescriptorSet> sets(inFlightCount * descriptorSet.size());
        VK_CHECK(vkAllocateDescriptorSets(m_Device, &descAllocInfo, sets.data()));
        for (size_t i = 0; i < descriptorSet.size(); i++)
        {
            descriptorSet[i]->m_DescriptorSets.resize(inFlightCount);
            for (size_t j = 0; j < inFlightCount; j++)
            {
                descriptorSet[i]->m_DescriptorSets[j] = sets[i * inFlightCount + j];
            }
        }
    }

    void ResourceCache::AllocateDescriptorSetArray(DescriptorSetArray* frameDescriptorSet, const DescriptorSetLayoutDesc& layoutDescription)
    {
        // TODO: support bindless model for InFlight Descriptor sets
        VkDescriptorSetLayout layout = GetDescriptorSetLayout(layoutDescription.LayoutBindings, layoutDescription.Flags);

        std::vector<VkDescriptorSetLayout> layouts(m_ImageCount);
        for (int i = 0; i < layouts.size(); i++)
            layouts[i] = layout;


        for (auto& binding : layoutDescription.LayoutBindings)
        {
            frameDescriptorSet->m_BindingTypes.push_back(binding.descriptorType);
        }

        VkDescriptorSetAllocateInfo descAllocInfo{};
        descAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        descAllocInfo.descriptorPool = m_DescriptorPool;
        descAllocInfo.descriptorSetCount = m_ImageCount;
        descAllocInfo.pSetLayouts = layouts.data();
        frameDescriptorSet->m_DescriptorSets.resize(m_ImageCount);

        VK_CHECK(vkAllocateDescriptorSets(m_Device, &descAllocInfo, frameDescriptorSet->m_DescriptorSets.data()));
    }

    void ResourceCache::CreateShader(Shader* shader, const ShaderDesc& shaderDesc)
    {
        if (shaderDesc.SpirV.size() == 0)
        {
            ARC_LOG_ERROR("SpirV size is 0: " + shaderDesc.FilePath);
            return;
        }

        VkShaderModuleCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        createInfo.codeSize = shaderDesc.SpirV.size() * sizeof(uint32_t);
        createInfo.pCode = reinterpret_cast<const uint32_t*>(shaderDesc.SpirV.data());

        VK_CHECK(vkCreateShaderModule(m_Device, &createInfo, nullptr, &shader->m_ShaderModule));

        shader->m_EntryPoint = shaderDesc.EntryPoint;
        shader->m_ShaderStage = VK_SHADER_STAGE_VERTEX_BIT;

        std::filesystem::path path = shaderDesc.FilePath;
        std::string extension = path.extension().string();

        VkShaderStageFlagBits shaderStage = VK_SHADER_STAGE_VERTEX_BIT;
        if (extension == ".vert")
            shaderStage = VK_SHADER_STAGE_VERTEX_BIT;
        if (extension == ".tcon")
            shaderStage = VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
        if (extension == ".teval")
            shaderStage = VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
        if (extension == ".geom")
            shaderStage = VK_SHADER_STAGE_GEOMETRY_BIT;
        if (extension == ".frag")
            shaderStage = VK_SHADER_STAGE_FRAGMENT_BIT;
        if (extension == ".comp")
            shaderStage = VK_SHADER_STAGE_COMPUTE_BIT;

        shader->m_ShaderStage = shaderStage;

        spirv_cross::Compiler comp(shaderDesc.SpirV);
        spirv_cross::ShaderResources resources = comp.get_shader_resources();
        //spv::ExecutionModel entryPoints = comp.get_execution_model();

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
            const auto& bufferType = comp.get_type(resource.base_type_id);
            uint32_t size = comp.get_declared_struct_size(bufferType);
            shader->m_PushConstantSize = size;
        }

        shader->m_StageOutputs = 0;
        for (const auto& output : resources.stage_outputs)
            shader->m_StageOutputs++;

        for (const auto& resource : resources.sampled_images)
        {
            const auto& bufferType = comp.get_type(resource.base_type_id);
            uint32_t set = comp.get_decoration(resource.id, spv::DecorationDescriptorSet);
            uint32_t binding = comp.get_decoration(resource.id, spv::DecorationBinding);
            const spirv_cross::SPIRType& type = comp.get_type(resource.type_id);
            uint32_t count = type.array[0];
            if (count <= 0) count = 1;

            Shader::DescriptorLayoutBinding layoutBinding;
            layoutBinding.setIndex = set;
            layoutBinding.binding.binding = binding;
            layoutBinding.binding.descriptorCount = count;
            layoutBinding.binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            layoutBinding.binding.stageFlags = static_cast<VkShaderStageFlags>(shaderStage);
            layoutBinding.binding.pImmutableSamplers = nullptr;
            shader->m_LayoutBindings.push_back(layoutBinding);
        }

        for (const auto& resource : resources.uniform_buffers)
        {
            const auto& bufferType = comp.get_type(resource.base_type_id);
            uint32_t bufferSize = comp.get_declared_struct_size(bufferType);
            uint32_t set = comp.get_decoration(resource.id, spv::DecorationDescriptorSet);
            uint32_t binding = comp.get_decoration(resource.id, spv::DecorationBinding);
            const spirv_cross::SPIRType& type = comp.get_type(resource.type_id);
            uint32_t count = type.array[0];
            if (count <= 0) count = 1;

            Shader::DescriptorLayoutBinding layoutBinding;
            layoutBinding.setIndex = set;
            layoutBinding.binding.binding = binding;
            layoutBinding.binding.descriptorCount = count;
            layoutBinding.binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            layoutBinding.binding.stageFlags = static_cast<VkShaderStageFlags>(shaderStage);
            layoutBinding.binding.pImmutableSamplers = nullptr;
            shader->m_LayoutBindings.push_back(layoutBinding);
        }

        for (const auto& resource : resources.storage_buffers)
        {
            const auto& bufferType = comp.get_type(resource.base_type_id);
            uint32_t bufferSize = comp.get_declared_struct_size(bufferType);
            uint32_t set = comp.get_decoration(resource.id, spv::DecorationDescriptorSet);
            uint32_t binding = comp.get_decoration(resource.id, spv::DecorationBinding);
            const spirv_cross::SPIRType& type = comp.get_type(resource.type_id);
            uint32_t count = type.array[0];
            if (count <= 0) count = 1;

            Shader::DescriptorLayoutBinding layoutBinding;
            layoutBinding.setIndex = set;
            layoutBinding.binding.binding = binding;
            layoutBinding.binding.descriptorCount = count;
            layoutBinding.binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            layoutBinding.binding.stageFlags = static_cast<VkShaderStageFlags>(shaderStage);
            layoutBinding.binding.pImmutableSamplers = nullptr;
            shader->m_LayoutBindings.push_back(layoutBinding);
        }

        for (const auto& resource : resources.separate_images)
        {
            const auto& bufferType = comp.get_type(resource.base_type_id);
            uint32_t set = comp.get_decoration(resource.id, spv::DecorationDescriptorSet);
            uint32_t binding = comp.get_decoration(resource.id, spv::DecorationBinding);
            const spirv_cross::SPIRType& type = comp.get_type(resource.type_id);
            uint32_t count = type.array[0];
            if (count <= 0) count = 1;

            Shader::DescriptorLayoutBinding layoutBinding;
            layoutBinding.setIndex = set;
            layoutBinding.binding.binding = binding;
            layoutBinding.binding.descriptorCount = count;
            layoutBinding.binding.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
            layoutBinding.binding.stageFlags = static_cast<VkShaderStageFlags>(shaderStage);
            layoutBinding.binding.pImmutableSamplers = nullptr;

            shader->m_LayoutBindings.push_back(layoutBinding);
        }

        for (const auto& resource : resources.storage_images)
        {
            const auto& bufferType = comp.get_type(resource.base_type_id);
            uint32_t set = comp.get_decoration(resource.id, spv::DecorationDescriptorSet);
            uint32_t binding = comp.get_decoration(resource.id, spv::DecorationBinding);
            const spirv_cross::SPIRType& type = comp.get_type(resource.type_id);
            uint32_t count = type.array[0];
            if (count <= 0) count = 1;

            Shader::DescriptorLayoutBinding layoutBinding;
            layoutBinding.setIndex = set;
            layoutBinding.binding.binding = binding;
            layoutBinding.binding.descriptorCount = count;
            layoutBinding.binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
            layoutBinding.binding.stageFlags = static_cast<VkShaderStageFlags>(shaderStage);
            layoutBinding.binding.pImmutableSamplers = nullptr;
            shader->m_LayoutBindings.push_back(layoutBinding);
        }

        for (const auto& resource : resources.separate_samplers)
        {
            const auto& bufferType = comp.get_type(resource.base_type_id);
            uint32_t set = comp.get_decoration(resource.id, spv::DecorationDescriptorSet);
            uint32_t binding = comp.get_decoration(resource.id, spv::DecorationBinding);
            const spirv_cross::SPIRType& type = comp.get_type(resource.type_id);
            uint32_t count = type.array[0];
            if (count <= 0) count = 1;

            Shader::DescriptorLayoutBinding layoutBinding;
            layoutBinding.setIndex = set;
            layoutBinding.binding.binding = binding;
            layoutBinding.binding.descriptorCount = count;
            layoutBinding.binding.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
            layoutBinding.binding.stageFlags = static_cast<VkShaderStageFlags>(shaderStage);
            layoutBinding.binding.pImmutableSamplers = nullptr;
            shader->m_LayoutBindings.push_back(layoutBinding);
        }

        m_ResourceReleaseFuctions[shader->m_ShaderModule] = [=]() {
            vkDestroyShaderModule(m_Device, shader->m_ShaderModule, nullptr);
            shader->m_LayoutBindings.clear();
        };
    }

    void ResourceCache::CreatePipeline(Pipeline* pipeline, const PipelineDesc& pipelineDesc)
    {
        std::vector<VkDynamicState> dynamicStates = {
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_SCISSOR
        };

        if (pipelineDesc.DepthBiasEnabled)
            dynamicStates.push_back(VK_DYNAMIC_STATE_DEPTH_BIAS);

        VkPipelineDynamicStateCreateInfo dynamicState = {};
        dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
        dynamicState.pDynamicStates = dynamicStates.data();

        VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
        vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertexInputInfo.vertexBindingDescriptionCount = static_cast<uint32_t>(pipelineDesc.VertexAttributes.InputBinding.size());
        vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(pipelineDesc.VertexAttributes.InputAttributes.size());
        vertexInputInfo.pVertexBindingDescriptions = pipelineDesc.VertexAttributes.InputBinding.data();
        vertexInputInfo.pVertexAttributeDescriptions = pipelineDesc.VertexAttributes.InputAttributes.data();

        VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo = {};
        inputAssemblyInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssemblyInfo.topology = static_cast<VkPrimitiveTopology>(pipelineDesc.Topology);
        inputAssemblyInfo.primitiveRestartEnable = VK_FALSE;

        VkExtent2D extent = { 0, 0 };

        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = extent.width;
        viewport.height = extent.height;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;

        VkRect2D scissor{};
        scissor.offset = { 0, 0 };
        scissor.extent = extent;

        VkPipelineViewportStateCreateInfo viewportInfo = {};
        viewportInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportInfo.viewportCount = 1;
        viewportInfo.pViewports = &viewport;
        viewportInfo.scissorCount = 1;
        viewportInfo.pScissors = &scissor;

        VkPipelineRasterizationStateCreateInfo rasterizationInfo = {};
        rasterizationInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizationInfo.depthClampEnable = VK_FALSE;
        rasterizationInfo.rasterizerDiscardEnable = VK_FALSE;
        rasterizationInfo.polygonMode = VK_POLYGON_MODE_FILL;
        rasterizationInfo.lineWidth = 1.0f;
        rasterizationInfo.cullMode = static_cast<VkCullModeFlags>(pipelineDesc.CullMode);
        rasterizationInfo.frontFace = VK_FRONT_FACE_CLOCKWISE;
        rasterizationInfo.depthBiasEnable = (pipelineDesc.DepthBiasEnabled == true) ? VK_TRUE : VK_FALSE;
        rasterizationInfo.depthBiasConstantFactor = 0.0f; // Optional
        rasterizationInfo.depthBiasClamp = 0.0f;          // Optional
        rasterizationInfo.depthBiasSlopeFactor = 0.0f;    // Optional

        VkPipelineMultisampleStateCreateInfo multisampleInfo = {};
        multisampleInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampleInfo.sampleShadingEnable = VK_FALSE;
        multisampleInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
        multisampleInfo.minSampleShading = 1.0f;             // Optional
        multisampleInfo.pSampleMask = nullptr;               // Optional
        multisampleInfo.alphaToCoverageEnable = VK_FALSE;    // Optional
        multisampleInfo.alphaToOneEnable = VK_FALSE;         // Optional

        VkPipelineDepthStencilStateCreateInfo depthStencilInfo = {};
        depthStencilInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        depthStencilInfo.depthTestEnable = (pipelineDesc.DepthTestEnabled == true) ? VK_TRUE : VK_FALSE;
        depthStencilInfo.depthWriteEnable = (pipelineDesc.DepthWriteEnabled == true) ? VK_TRUE : VK_FALSE;
        depthStencilInfo.depthCompareOp = static_cast<VkCompareOp>(pipelineDesc.DepthCompare);
        depthStencilInfo.depthBoundsTestEnable = VK_FALSE;
        depthStencilInfo.minDepthBounds = 0.0f;
        depthStencilInfo.maxDepthBounds = 1.0f;
        depthStencilInfo.stencilTestEnable = VK_FALSE;
        depthStencilInfo.front = {};
        depthStencilInfo.back = {};

        uint32_t colorAttachmentCount = 0;
        uint32_t pushConstantSize = 0;
        for (const auto& shader : pipelineDesc.ShaderStages)
        {
            if (shader->m_ShaderStage == VK_SHADER_STAGE_FRAGMENT_BIT)
                colorAttachmentCount = shader->m_StageOutputs;
            if (shader->m_PushConstantSize > pushConstantSize)
                pushConstantSize = shader->m_PushConstantSize;
        }

        std::vector<VkPipelineColorBlendAttachmentState> colorBlendAttachment;
        for (int i = 0; i < colorAttachmentCount; i++)
        {
            VkPipelineColorBlendAttachmentState tempAttachment;
            tempAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
            tempAttachment.blendEnable = VK_TRUE;
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
        colorBlendInfo.attachmentCount = colorBlendAttachment.size();
        colorBlendInfo.pAttachments = colorBlendAttachment.data();
        colorBlendInfo.blendConstants[0] = 0.0f;
        colorBlendInfo.blendConstants[1] = 0.0f;
        colorBlendInfo.blendConstants[2] = 0.0f;
        colorBlendInfo.blendConstants[3] = 0.0f;

        VkPushConstantRange pushConstantRange{};
        pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
        pushConstantRange.offset = 0;
        pushConstantRange.size = pushConstantSize;

        std::map<uint32_t, std::map<uint32_t, VkDescriptorSetLayoutBinding>> bindings;
        for (const auto& shader : pipelineDesc.ShaderStages)
        {
            for (auto& b : shader->m_LayoutBindings)
            {
                if (bindings[b.setIndex].contains(b.binding.binding))
                {
                    bindings[b.setIndex][b.binding.binding].stageFlags |= b.binding.stageFlags;
                }
                else
                {
                    bindings[b.setIndex][b.binding.binding] = b.binding;
                }
            }
        }

        std::vector<VkDescriptorSetLayout> layouts;
        for (auto& set : bindings)
        {
            uint32_t flags = 0;
            std::vector<VkDescriptorSetLayoutBinding> layoutBindings;
            for (auto& binding : set.second)
            {
                layoutBindings.push_back(binding.second);
                if (binding.second.descriptorCount >= MAX_BINDLESS_DESCRIPTOR_COUNT)
                    flags |= (uint32_t)DescriptorFlags::Bindless;
            }

            layouts.push_back(GetDescriptorSetLayout(layoutBindings, flags));
        }

        VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = layouts.size();
        pipelineLayoutInfo.pSetLayouts = layouts.data();
        if (pushConstantSize > 0)
        {
            pipelineLayoutInfo.pushConstantRangeCount = 1;
            pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;
        }

        VK_CHECK(vkCreatePipelineLayout(m_Device, &pipelineLayoutInfo, nullptr, &pipeline->m_PipelineLayout));

        std::vector<VkPipelineShaderStageCreateInfo> shaderStages(pipelineDesc.ShaderStages.size());
        for (int i = 0; i < shaderStages.size(); i++)
        {
            shaderStages[i] = {};
            shaderStages[i].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            shaderStages[i].stage = pipelineDesc.ShaderStages[i]->m_ShaderStage;
            shaderStages[i].module = pipelineDesc.ShaderStages[i]->m_ShaderModule;
            shaderStages[i].pName = pipelineDesc.ShaderStages[i]->m_EntryPoint.c_str();
            shaderStages[i].flags = 0;
            shaderStages[i].pNext = nullptr;
            shaderStages[i].pSpecializationInfo = nullptr;
        }

        VkGraphicsPipelineCreateInfo pipelineInfo = {};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineInfo.stageCount = pipelineDesc.ShaderStages.size();
        pipelineInfo.pStages = shaderStages.data();

        pipelineInfo.pVertexInputState = &vertexInputInfo;
        pipelineInfo.pInputAssemblyState = &inputAssemblyInfo;
        pipelineInfo.pViewportState = &viewportInfo;
        pipelineInfo.pRasterizationState = &rasterizationInfo;
        pipelineInfo.pMultisampleState = &multisampleInfo;
        pipelineInfo.pDepthStencilState = &depthStencilInfo;
        pipelineInfo.pColorBlendState = &colorBlendInfo;
        pipelineInfo.pDynamicState = &dynamicState;

        pipelineInfo.layout = pipeline->m_PipelineLayout;// configInfo.pipelineLayout;
        pipelineInfo.renderPass = pipelineDesc.RenderPass;
        pipelineInfo.subpass = 0;// info.subpassIndex;

        pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
        pipelineInfo.basePipelineIndex = -1;

        VK_CHECK(vkCreateGraphicsPipelines(m_Device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline->m_Pipeline));

        m_ResourceReleaseFuctions[pipeline->m_Pipeline] = [=]() {
            vkDestroyPipelineLayout(m_Device, pipeline->m_PipelineLayout, nullptr);
            vkDestroyPipeline(m_Device, pipeline->m_Pipeline, nullptr);
        };
    }

    void ResourceCache::CreateComputePipeline(ComputePipeline* pipeline, const ComputePipelineDesc& pipelineDesc)
    {

        VkPushConstantRange pushConstantRange{};
        pushConstantRange.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
        pushConstantRange.offset = 0;
        pushConstantRange.size = pipelineDesc.ShaderStage->m_PushConstantSize;

        std::array<std::vector<VkDescriptorSetLayoutBinding>, 32> layoutBindings;
        int32_t maxSetIndex = -1;
        for (auto& b : pipelineDesc.ShaderStage->m_LayoutBindings)
        {
            layoutBindings[b.setIndex].push_back(b.binding);
            if (b.setIndex > maxSetIndex)
                maxSetIndex = b.setIndex;
        }
        std::vector<VkDescriptorSetLayoutCreateInfo> descriptorSetLayoutInfo(maxSetIndex + 1);
        std::vector<VkDescriptorSetLayout> layouts(maxSetIndex + 1);
        for (size_t i = 0; i < descriptorSetLayoutInfo.size(); i++)
        {
            layouts[i] = GetDescriptorSetLayout(layoutBindings[i], 0);
        }

        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = layouts.size();
        pipelineLayoutInfo.pSetLayouts = layouts.data();
        if (pipelineDesc.ShaderStage->m_PushConstantSize > 0)
        {
            pipelineLayoutInfo.pushConstantRangeCount = 1;
            pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;
        }

        VK_CHECK(vkCreatePipelineLayout(m_Device, &pipelineLayoutInfo, nullptr, &pipeline->m_PipelineLayout));

        VkPipelineShaderStageCreateInfo shaderStage = {};
        shaderStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        shaderStage.stage = pipelineDesc.ShaderStage->m_ShaderStage;
        shaderStage.module = pipelineDesc.ShaderStage->m_ShaderModule;
        shaderStage.pName = pipelineDesc.ShaderStage->m_EntryPoint.c_str();
        shaderStage.flags = 0;
        shaderStage.pNext = nullptr;
        shaderStage.pSpecializationInfo = nullptr;

        VkComputePipelineCreateInfo pipelineInfo{};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
        pipelineInfo.layout = pipeline->m_PipelineLayout;
        pipelineInfo.stage = shaderStage;

        VK_CHECK(vkCreateComputePipelines(m_Device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline->m_Pipeline));

        m_ResourceReleaseFuctions[pipeline->m_Pipeline] = [=]() {
            vkDestroyPipelineLayout(m_Device, pipeline->m_PipelineLayout, nullptr);
            vkDestroyPipeline(m_Device, pipeline->m_Pipeline, nullptr);
        };
    }

    VkDescriptorSetLayout ResourceCache::GetDescriptorSetLayout(const std::vector<VkDescriptorSetLayoutBinding>& layoutBindings, uint32_t flags)
    {
        uint64_t layoutKey = 0;
        if (flags & (uint32_t)DescriptorFlags::Bindless)
            layoutKey = (uint32_t)DescriptorFlags::Bindless;

        for (size_t i = 0; i < layoutBindings.size(); i++)
        {
            uint64_t type = static_cast<uint64_t>(layoutBindings[i].descriptorType);
            layoutKey |= type << (i * 4 + 4);
        }

        if (m_DescriptorSetLayouts.find(layoutKey) == m_DescriptorSetLayouts.end())
        {
            VkDescriptorSetLayoutCreateInfo createInfo{};
            createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
            createInfo.pNext = nullptr;
            createInfo.bindingCount = static_cast<uint32_t>(layoutBindings.size());
            createInfo.pBindings = layoutBindings.data();
            createInfo.flags = 0;

            VkDescriptorSetLayoutBindingFlagsCreateInfoEXT extendedInfo{};
            VkDescriptorBindingFlags bindless_flags;
            if (flags & (uint32_t)DescriptorFlags::Bindless)
            {
                bindless_flags = VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT_EXT | VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT_EXT | VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT_EXT;
                extendedInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO_EXT;
                extendedInfo.pNext = nullptr;
                extendedInfo.bindingCount = 1;
                extendedInfo.pBindingFlags = &bindless_flags;

                createInfo.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT_EXT;
                createInfo.pNext = &extendedInfo;
            }

            VkDescriptorSetLayout layout;
            VK_CHECK(vkCreateDescriptorSetLayout(
                m_Device,
                &createInfo,
                nullptr,
                &layout));

            m_DescriptorSetLayouts[layoutKey] = layout;

            return layout;
        }
        else
        {
            return m_DescriptorSetLayouts[layoutKey];
        }
    }

    void* ResourceCache::MapMemory(GpuBuffer* buffer)
    {
        void* data;
        vmaMapMemory(m_MemoryAllocator, buffer->m_Allocation, &data);
        return data;
    }

    void ResourceCache::UnmapMemory(GpuBuffer* buffer)
    {
        vmaUnmapMemory(m_MemoryAllocator, buffer->m_Allocation);
    }

    void* ResourceCache::MapMemory(GpuBufferSet* buffer, uint32_t frameIndex)
    {
        void* data;
        vmaMapMemory(m_MemoryAllocator, buffer->m_Allocation[frameIndex], &data);
        return data;
    }

    void ResourceCache::UnmapMemory(GpuBufferSet* buffer, uint32_t frameIndex)
    {
        vmaUnmapMemory(m_MemoryAllocator, buffer->m_Allocation[frameIndex]);
    }

    void ResourceCache::ReleaseResource(GpuBuffer* buffer)
    {
        void* key = buffer->m_Buffer;
        m_ResourceReleaseFuctions[key]();
        m_ResourceReleaseFuctions.erase(key);
    }

    void ResourceCache::ReleaseResource(GpuBufferSet* buffer)
    {
        void* key = buffer->m_Buffer[0];
        m_ResourceReleaseFuctions[key]();
        m_ResourceReleaseFuctions.erase(key);
    }

    void ResourceCache::ReleaseResource(Image* image)
    {
        void* key = image->m_Image;
        m_ResourceReleaseFuctions[key]();
        m_ResourceReleaseFuctions.erase(key);
    }

    void ResourceCache::ReleaseResource(Sampler* sampler)
    {
        void* key = sampler->m_Sampler;
        m_ResourceReleaseFuctions[key]();
        m_ResourceReleaseFuctions.erase(key);
    }

    void ResourceCache::ReleaseResource(Shader* shader)
    {
        void* key = shader->m_ShaderModule;
        m_ResourceReleaseFuctions[key]();
        m_ResourceReleaseFuctions.erase(key);
    }

    void ResourceCache::ReleaseResource(Pipeline* pipeline)
    {
        void* key = pipeline->m_Pipeline;
        m_ResourceReleaseFuctions[key]();
        m_ResourceReleaseFuctions.erase(key);
    }

    void ResourceCache::ReleaseResource(ComputePipeline* pipeline)
    {
        void* key = pipeline->m_Pipeline;
        m_ResourceReleaseFuctions[key]();
        m_ResourceReleaseFuctions.erase(key);
    }

    void ResourceCache::FreeResources()
    {
        for (auto& layout : m_DescriptorSetLayouts)
        {
            vkDestroyDescriptorSetLayout(m_Device, layout.second, nullptr);
        }

        m_DescriptorSetLayouts.clear();

        for (auto& [handle, func] : m_ResourceReleaseFuctions)
        { 
            func();
        }
        
        m_ResourceReleaseFuctions.clear();
    }

    void ResourceCache::PrintHeapBudgets()
    {
        const VkPhysicalDeviceMemoryProperties* properties;
        vmaGetMemoryProperties(m_MemoryAllocator, &properties);
        uint32_t heapCount = properties->memoryHeapCount;
        std::vector<VmaBudget> budgets(heapCount);
        vmaGetHeapBudgets(m_MemoryAllocator, budgets.data());

        uint32_t allocationCount = 0;
        uint64_t allocationBytes = 0;
        for (int heapIndex = 0; heapIndex < heapCount; heapIndex++)
        {
            allocationCount += budgets[heapIndex].statistics.allocationCount;
            allocationBytes += budgets[heapIndex].statistics.allocationBytes;
            
            printf("My heap currently has %u allocations taking %lf MB,\n",
                budgets[heapIndex].statistics.allocationCount,
                budgets[heapIndex].statistics.allocationBytes / 1048576.0);
            printf("allocated out of %u Vulkan device memory blocks taking %lf MB,\n",
                budgets[heapIndex].statistics.blockCount,
                budgets[heapIndex].statistics.blockBytes / 1048576.0);
            printf("Vulkan reports total usage %lf MB with budget %lf MB.\n",
                budgets[heapIndex].usage / 1048576.0,
                budgets[heapIndex].budget / 1048576.0);
            
        }
        double allocationMegaBytes = allocationBytes / 1048576.0;
        std::cout << "Total " << allocationCount << " allocations taking " << allocationMegaBytes << " MB\n";
    }
}