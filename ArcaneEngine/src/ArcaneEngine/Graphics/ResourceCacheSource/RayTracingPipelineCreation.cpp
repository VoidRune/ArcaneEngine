#include "ArcaneEngine/Graphics/ResourceCache.h"
#include "ArcaneEngine/Core/Log.h"
#include "ArcaneEngine/Graphics/VulkanCore/VulkanLocal.h"
#include <vulkan/vulkan_core.h>
#include <vk_mem_alloc.h>
#include <map>

namespace Arc
{

    Shader* GetShaderWithStage(ShaderStage stage, const std::vector<Shader*>& shaders)
    {
        for (auto& s : shaders)
        {
            if (s->GetShaderStage() == stage)
            {
                return s;
            }
        }
        return nullptr;
    }

    static inline uint32_t align_u32(uint32_t v, uint32_t a) { return (v + a - 1) & ~(a - 1); }
    static inline uint64_t align_u64(uint64_t v, uint64_t a) { return (v + a - 1) & ~(a - 1); }

    extern PFN_vkCreateRayTracingPipelinesKHR vkCreateRayTracingPipelinesKHR;
    extern PFN_vkGetRayTracingShaderGroupHandlesKHR  vkGetRayTracingShaderGroupHandlesKHR;
    extern VkDescriptorSetLayout GetDescriptorSetLayout(VkDevice device, std::unordered_map<uint64_t, DescriptorSetLayoutHandle>& map, const std::vector<VkDescriptorSetLayoutBinding>& bindings, uint32_t flags);

    void ResourceCache::CreateRayTracingPipeline(RayTracingPipeline* pipeline, const RayTracingPipelineDesc& desc)
    {
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

        uint32_t pushConstantSize = 0;
        for (const auto& shader : desc.ShaderStages)
        {
            if (shader->m_PushConstantSize > pushConstantSize)
                pushConstantSize = shader->m_PushConstantSize;
        }

        VkPushConstantRange pushConstantRange{};
        pushConstantRange.stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VK_SHADER_STAGE_MISS_BIT_KHR;
        pushConstantRange.offset = 0;
        pushConstantRange.size = pushConstantSize;

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

        std::vector<VkPipelineShaderStageCreateInfo> shaderStages;
        std::vector<VkRayTracingShaderGroupCreateInfoKHR> shaderGroups;

        // Ray generation group
        {
            Shader* shader = GetShaderWithStage(ShaderStage::RayGen, desc.ShaderStages);
            if (shader == nullptr)
            {
                ARC_LOG_ERROR("RayTracingPipelineDesc does not contain shader with RayGen shaders stage!");
            }
            VkPipelineShaderStageCreateInfo stage{};
            stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            stage.flags = 0;
            stage.stage = VK_SHADER_STAGE_RAYGEN_BIT_KHR;
            stage.module = (VkShaderModule)shader->m_Module;
            stage.pName = shader->m_EntryPoint.c_str();
            stage.pSpecializationInfo = nullptr;
            shaderStages.push_back(stage);

            VkRayTracingShaderGroupCreateInfoKHR shaderGroup{};
            shaderGroup.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
            shaderGroup.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
            shaderGroup.generalShader = static_cast<uint32_t>(shaderStages.size()) - 1;
            shaderGroup.closestHitShader = VK_SHADER_UNUSED_KHR;
            shaderGroup.anyHitShader = VK_SHADER_UNUSED_KHR;
            shaderGroup.intersectionShader = VK_SHADER_UNUSED_KHR;
            shaderGroups.push_back(shaderGroup);
        }

        // Miss group
        {
            Shader* shader = GetShaderWithStage(ShaderStage::RayMiss, desc.ShaderStages);
            if (shader == nullptr)
            {
                ARC_LOG_ERROR("RayTracingPipelineDesc does not contain shader with RayMiss shaders stage!");
            }
            VkPipelineShaderStageCreateInfo stage{};
            stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            stage.flags = 0;
            stage.stage = VK_SHADER_STAGE_MISS_BIT_KHR;
            stage.module = (VkShaderModule)shader->m_Module;
            stage.pName = shader->m_EntryPoint.c_str();
            stage.pSpecializationInfo = nullptr;
            shaderStages.push_back(stage);

            VkRayTracingShaderGroupCreateInfoKHR shaderGroup{};
            shaderGroup.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
            shaderGroup.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
            shaderGroup.generalShader = static_cast<uint32_t>(shaderStages.size()) - 1;
            shaderGroup.closestHitShader = VK_SHADER_UNUSED_KHR;
            shaderGroup.anyHitShader = VK_SHADER_UNUSED_KHR;
            shaderGroup.intersectionShader = VK_SHADER_UNUSED_KHR;
            shaderGroups.push_back(shaderGroup);
        }

        // Closest hit group
        {
            Shader* shader = GetShaderWithStage(ShaderStage::RayClosestHit, desc.ShaderStages);
            if (shader == nullptr)
            {
                ARC_LOG_ERROR("RayTracingPipelineDesc does not contain shader with RayClosestHit shaders stage!");
            }
            VkPipelineShaderStageCreateInfo stage{};
            stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            stage.flags = 0;
            stage.stage = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;
            stage.module = (VkShaderModule)shader->m_Module;
            stage.pName = shader->m_EntryPoint.c_str();
            stage.pSpecializationInfo = nullptr;
            shaderStages.push_back(stage);

            VkRayTracingShaderGroupCreateInfoKHR shaderGroup{};
            shaderGroup.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
            shaderGroup.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR;
            shaderGroup.generalShader = VK_SHADER_UNUSED_KHR;
            shaderGroup.closestHitShader = static_cast<uint32_t>(shaderStages.size()) - 1;
            shaderGroup.anyHitShader = VK_SHADER_UNUSED_KHR;
            shaderGroup.intersectionShader = VK_SHADER_UNUSED_KHR;
            shaderGroups.push_back(shaderGroup);
        }

        VkRayTracingPipelineCreateInfoKHR rayTracingPipelineCI{};
        rayTracingPipelineCI.sType = VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR;
        rayTracingPipelineCI.stageCount = static_cast<uint32_t>(shaderStages.size());
        rayTracingPipelineCI.pStages = shaderStages.data();
        rayTracingPipelineCI.groupCount = static_cast<uint32_t>(shaderGroups.size());
        rayTracingPipelineCI.pGroups = shaderGroups.data();
        rayTracingPipelineCI.maxPipelineRayRecursionDepth = 1;
        rayTracingPipelineCI.layout = pipelineLayout;

        VkPipeline rtPipeline;
        VK_CHECK(vkCreateRayTracingPipelinesKHR((VkDevice)m_LogicalDevice, VK_NULL_HANDLE, VK_NULL_HANDLE, 1, &rayTracingPipelineCI, nullptr, &rtPipeline));


        VkPhysicalDeviceRayTracingPipelinePropertiesKHR rayProps{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR };
        VkPhysicalDeviceProperties2 pdprops{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2 }; 
        pdprops.pNext = &rayProps;
        vkGetPhysicalDeviceProperties2((VkPhysicalDevice)m_PhysicalDevice, &pdprops);
        uint32_t handleSize = rayProps.shaderGroupHandleSize;
        uint32_t handleAlign = rayProps.shaderGroupHandleAlignment;
        uint32_t baseAlign = rayProps.shaderGroupBaseAlignment;

        uint32_t groupCount = shaderGroups.size();
        std::vector<uint8_t> handles(groupCount * handleSize);
        vkGetRayTracingShaderGroupHandlesKHR((VkDevice)m_LogicalDevice, rtPipeline, 0, groupCount, handles.size(), handles.data());

        uint32_t sbtRecordSize = align_u64(handleSize, handleAlign);

        uint64_t raygenRegionSize = align_u64(sbtRecordSize * 1, baseAlign);
        uint64_t missRegionOffset = raygenRegionSize;
        uint64_t missRegionSize = align_u64(sbtRecordSize * 1, baseAlign);
        uint64_t hitRegionOffset = missRegionOffset + missRegionSize;
        uint64_t hitRegionSize = align_u64(sbtRecordSize * 1, baseAlign);

        uint64_t sbtSize = hitRegionOffset + hitRegionSize;

        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = sbtSize;
        bufferInfo.usage = VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        VmaAllocationCreateInfo allocInfo = {};
        allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
        allocInfo.requiredFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
        allocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;

        VkBuffer buffer;
        VmaAllocation allocation;
        VK_CHECK(vmaCreateBuffer((VmaAllocator)m_Allocator, &bufferInfo, &allocInfo,
            &buffer,
            &allocation,
            nullptr));


        void* data;
        vmaMapMemory((VmaAllocator)m_Allocator, allocation, &data);
        memcpy((uint8_t*)data + 0 * sbtRecordSize, handles.data() + 0 * handleSize, handleSize);
        memcpy((uint8_t*)data + raygenRegionSize + 0 * sbtRecordSize, handles.data() + 1 * handleSize, handleSize);
        memcpy((uint8_t*)data + raygenRegionSize + missRegionSize + 0 * sbtRecordSize, handles.data() + 2 * handleSize, handleSize);
        vmaUnmapMemory((VmaAllocator)m_Allocator, allocation);

        VkBufferDeviceAddressInfo addrInfo{ VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO };
        addrInfo.buffer = buffer;
        VkDeviceAddress address = vkGetBufferDeviceAddress((VkDevice)m_LogicalDevice, &addrInfo);

        pipeline->m_RayGenShaderBindingTable.DeviceAddress = address + 0;
        pipeline->m_RayGenShaderBindingTable.Stride = sbtRecordSize;
        pipeline->m_RayGenShaderBindingTable.Size = sbtRecordSize;
        pipeline->m_RayMissShaderBindingTable.DeviceAddress = address + raygenRegionSize;
        pipeline->m_RayMissShaderBindingTable.Stride = sbtRecordSize;
        pipeline->m_RayMissShaderBindingTable.Size = missRegionSize;
        pipeline->m_RayClosestHitShaderBindingTable.DeviceAddress = address + raygenRegionSize + missRegionSize;
        pipeline->m_RayClosestHitShaderBindingTable.Stride = sbtRecordSize;
        pipeline->m_RayClosestHitShaderBindingTable.Size = hitRegionSize;

        pipeline->m_Pipeline = rtPipeline;
        pipeline->m_PipelineLayout = pipelineLayout;
        pipeline->m_ShaderBindingTableBuffer = buffer;
        pipeline->m_ShaderBindingTableAllocation = allocation;
        m_Resources[pipeline] = ResourceType::RayTracingPipeline;
    }

    void ResourceCache::ReleaseResource(RayTracingPipeline* raytracingPipeline)
    {
        if (!m_Resources.contains(raytracingPipeline))
        {
            ARC_LOG_FATAL("Cannot find RayTracingPipeline resource to release!");
        }
        vmaDestroyBuffer((VmaAllocator)m_Allocator, (VkBuffer)raytracingPipeline->m_ShaderBindingTableBuffer, (VmaAllocation)raytracingPipeline->m_ShaderBindingTableAllocation);
        vkDestroyPipelineLayout((VkDevice)m_LogicalDevice, (VkPipelineLayout)raytracingPipeline->m_PipelineLayout, nullptr);
        vkDestroyPipeline((VkDevice)m_LogicalDevice, (VkPipeline)raytracingPipeline->m_Pipeline, nullptr);
        m_Resources.erase(raytracingPipeline);
    }
}