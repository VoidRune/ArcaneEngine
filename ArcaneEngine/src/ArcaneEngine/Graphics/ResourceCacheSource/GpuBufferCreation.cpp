#include "ArcaneEngine/Graphics/ResourceCache.h"
#include "ArcaneEngine/Core/Log.h"
#include "ArcaneEngine/Graphics/VulkanCore/VulkanLocal.h"
#include <vulkan/vulkan_core.h>
#include <vk_mem_alloc.h>

namespace Arc
{
	void ResourceCache::CreateGpuBuffer(GpuBuffer* gpuBuffer, const GpuBufferDesc& desc)
	{
        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = desc.Size;
        bufferInfo.usage = (VkBufferUsageFlags)desc.UsageFlags;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        VmaAllocationCreateInfo allocInfo = {};
        allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
        allocInfo.requiredFlags = (VkMemoryPropertyFlags)desc.MemoryProperty;
        if (allocInfo.requiredFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT ||
            allocInfo.requiredFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT ||
            allocInfo.requiredFlags & VK_MEMORY_PROPERTY_HOST_CACHED_BIT)
        {
            allocInfo.flags |= VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
        }

        VkBuffer buffer;
        VmaAllocation allocation;
        VK_CHECK(vmaCreateBuffer((VmaAllocator)m_Allocator, &bufferInfo, &allocInfo,
            &buffer,
            &allocation,
            nullptr));
        gpuBuffer->m_Buffer = buffer;
        gpuBuffer->m_Allocation = allocation;
        gpuBuffer->m_Size = desc.Size;

        m_Resources[gpuBuffer] = ResourceType::GpuBuffer;
	}

    void ResourceCache::ReleaseResource(GpuBuffer* gpuBuffer)
    {
        if (!m_Resources.contains(gpuBuffer))
        {
            ARC_LOG_FATAL("Cannot find GpuBuffer resource to release!");
        }
        vmaDestroyBuffer((VmaAllocator)m_Allocator, (VkBuffer)gpuBuffer->m_Buffer, (VmaAllocation)gpuBuffer->m_Allocation);
        m_Resources.erase(gpuBuffer);
    }

    void ResourceCache::CreateGpuBufferArray(GpuBufferArray* gpuBufferArray, const GpuBufferDesc& desc)
    {
        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = desc.Size;
        bufferInfo.usage = (VkBufferUsageFlags)desc.UsageFlags;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        VmaAllocationCreateInfo allocInfo = {};
        allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
        allocInfo.requiredFlags = (VkMemoryPropertyFlags)desc.MemoryProperty;
        if (allocInfo.requiredFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT ||
            allocInfo.requiredFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT ||
            allocInfo.requiredFlags & VK_MEMORY_PROPERTY_HOST_CACHED_BIT)
        {
            allocInfo.flags |= VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
        }

        gpuBufferArray->m_Buffers.resize(m_FramesInFlight);
        gpuBufferArray->m_Allocations.resize(m_FramesInFlight);
        gpuBufferArray->m_Size = desc.Size;
        for (int i = 0; i < m_FramesInFlight; i++)
        {
            VkBuffer buffer;
            VmaAllocation allocation;
            VK_CHECK(vmaCreateBuffer((VmaAllocator)m_Allocator, &bufferInfo, &allocInfo,
                &buffer,
                &allocation,
                nullptr));
            gpuBufferArray->m_Buffers[i] = buffer;
            gpuBufferArray->m_Allocations[i] = allocation;
        }

        m_Resources[gpuBufferArray] = ResourceType::GpuBufferArray;
    }

    void ResourceCache::ReleaseResource(GpuBufferArray* gpuBufferArray)
    {
        if (!m_Resources.contains(gpuBufferArray))
        {
            ARC_LOG_FATAL("Cannot find GpuBufferArray resource to release!");
        }
        for (int i = 0; i < gpuBufferArray->m_Buffers.size(); i++)
        {
            vmaDestroyBuffer((VmaAllocator)m_Allocator, (VkBuffer)gpuBufferArray->m_Buffers[i], (VmaAllocation)gpuBufferArray->m_Allocations[i]);
        }
        m_Resources.erase(gpuBufferArray);
    }
}