#include "ResourceCache.h"
#include "Device.h"
#include "VulkanCore/VulkanLocal.h"
#include "ArcaneEngine/Core/Log.h"

#define VMA_IMPLEMENTATION
#include "vk_mem_alloc.h"

namespace Arc
{
	ResourceCache::ResourceCache(Device* device)
	{
        if (!device)
        {
            ARC_LOG_FATAL("Failed to create ResourceCache object: Device pointer is not valid!");
        }
        m_Device = device;
        m_LogicalDevice = device->GetLogicalDevice();
        m_PhysicalDevice = device->GetPhysicalDevice();
        m_FramesInFlight = device->GetFramesInFlightCount();
        m_GraphicsQueue = device->GetGraphicsQueue();
        m_CommandPool = device->GetCommanPool();

        VmaAllocatorCreateInfo allocatorInfo = {};
        allocatorInfo.physicalDevice = (VkPhysicalDevice)m_PhysicalDevice;
        allocatorInfo.device = (VkDevice)m_LogicalDevice;
        allocatorInfo.instance = (VkInstance)device->GetInstance();
        allocatorInfo.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
        VmaAllocator allocator;
        VK_CHECK(vmaCreateAllocator(&allocatorInfo, &allocator));
        m_Allocator = allocator;

        std::vector<VkDescriptorPoolSize> poolSizes =
        {
            {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1024},
            {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1024},
            {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1024},
            {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1024},
            {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1024},
            {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1024},
            {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1024},
        };

        VkDescriptorPoolCreateInfo descriptorPoolInfo{};
        descriptorPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        descriptorPoolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
        descriptorPoolInfo.pPoolSizes = poolSizes.data();
        descriptorPoolInfo.maxSets = 1024;
        descriptorPoolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT_EXT;

        VkDescriptorPool descriptorPool;
        VK_CHECK(vkCreateDescriptorPool((VkDevice)m_LogicalDevice, &descriptorPoolInfo, nullptr, &descriptorPool));
        m_DescriptorPool = descriptorPool;
	}

	ResourceCache::~ResourceCache()
	{
        FreeResources();
        vkDestroyDescriptorPool((VkDevice)m_LogicalDevice, (VkDescriptorPool)m_DescriptorPool, nullptr);
        vmaDestroyAllocator((VmaAllocator)m_Allocator);
	}

    void* ResourceCache::MapMemory(GpuBuffer* buffer)
    {
        void* data;
        vmaMapMemory((VmaAllocator)m_Allocator, (VmaAllocation)buffer->m_Allocation, &data);
        return data;
    }

    void ResourceCache::UnmapMemory(GpuBuffer* buffer)
    {
        vmaUnmapMemory((VmaAllocator)m_Allocator, (VmaAllocation)buffer->m_Allocation);
    }

    void* ResourceCache::MapMemory(GpuBufferArray* bufferArray, uint32_t frameIndex)
    {
        void* data;
        vmaMapMemory((VmaAllocator)m_Allocator, (VmaAllocation)bufferArray->m_Allocations[frameIndex], &data);
        return data;
    }

    void ResourceCache::UnmapMemory(GpuBufferArray* bufferArray, uint32_t frameIndex)
    {
        vmaUnmapMemory((VmaAllocator)m_Allocator, (VmaAllocation)bufferArray->m_Allocations[frameIndex]);
    }

    void ResourceCache::FreeResources()
    {
        for (auto& [key, layout] : m_DescriptorSetLayouts)
        {
            vkDestroyDescriptorSetLayout((VkDevice)m_LogicalDevice, (VkDescriptorSetLayout)layout, nullptr);
        }
        m_DescriptorSetLayouts.clear();

        for (auto& [resource, type] : m_Resources)
        {
            switch (type)
            {
            case ResourceType::GpuBuffer:
            {
                ReleaseResource((GpuBuffer*)resource);
            }
            break;
            case ResourceType::GpuBufferArray:
            {
                ReleaseResource((GpuBufferArray*)resource);
            }
            break;
            case ResourceType::GpuImage:
            {
                ReleaseResource((GpuImage*)resource);
            }
            break;
            case ResourceType::Sampler:
            {
                ReleaseResource((Sampler*)resource);
            }
            break;
            case ResourceType::Shader:
            {
                ReleaseResource((Shader*)resource);
            }
            break;
            case ResourceType::Pipeline:
            {
                ReleaseResource((Pipeline*)resource);
            }
            break;
            case ResourceType::ComputePipeline:
            {
                ReleaseResource((ComputePipeline*)resource);
            }
            break;
            case ResourceType::BottomLevelAS:
            {
                ReleaseResource((BottomLevelAS*)resource);
            }
            break;
            case ResourceType::TopLevelAS:
            {
                ReleaseResource((TopLevelAS*)resource);
            }
            break;
            case ResourceType::RayTracingPipeline:
            {
                ReleaseResource((RayTracingPipeline*)resource);
            }
            break;
            }
        }
        m_Resources.clear();
    }

    void ResourceCache::PrintHeapBudgets()
    {
        const VkPhysicalDeviceMemoryProperties* properties;
        vmaGetMemoryProperties((VmaAllocator)m_Allocator, &properties);
        uint32_t heapCount = properties->memoryHeapCount;
        std::vector<VmaBudget> budgets(heapCount);
        vmaGetHeapBudgets((VmaAllocator)m_Allocator, budgets.data());

        uint32_t allocationCount = 0;
        uint64_t allocationBytes = 0;
        for (uint32_t heapIndex = 0; heapIndex < heapCount; heapIndex++)
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