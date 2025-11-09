#include "ArcaneEngine/Graphics/ResourceCache.h"
#include "ArcaneEngine/Core/Log.h"
#include "ArcaneEngine/Graphics/VulkanCore/VulkanLocal.h"
#include "ArcaneEngine/Graphics/Device.h"
#include <vulkan/vulkan_core.h>
#include <vk_mem_alloc.h>
#include <map>

namespace Arc
{
    extern PFN_vkCreateAccelerationStructureKHR        vkCreateAccelerationStructureKHR;
    extern PFN_vkDestroyAccelerationStructureKHR       vkDestroyAccelerationStructureKHR;
    extern PFN_vkCmdBuildAccelerationStructuresKHR     vkCmdBuildAccelerationStructuresKHR;
    extern PFN_vkGetAccelerationStructureBuildSizesKHR vkGetAccelerationStructureBuildSizesKHR;
    extern PFN_vkGetAccelerationStructureDeviceAddressKHR vkGetAccelerationStructureDeviceAddressKHR;

    void ResourceCache::CreateBottomLevelAS(BottomLevelAS* bottomLevelAS, const BottomLevelASDesc& desc)
    {
        VkDeviceOrHostAddressConstKHR vertexBufferDeviceAddress{};
        VkDeviceOrHostAddressConstKHR indexBufferDeviceAddress{};

        VkBufferDeviceAddressInfo addrInfo{ VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO };
        addrInfo.buffer = (VkBuffer)desc.VertexBuffer;
        vertexBufferDeviceAddress.deviceAddress = vkGetBufferDeviceAddress((VkDevice)m_LogicalDevice, &addrInfo);
        addrInfo.buffer = (VkBuffer)desc.IndexBuffer;
        indexBufferDeviceAddress.deviceAddress = vkGetBufferDeviceAddress((VkDevice)m_LogicalDevice, &addrInfo);

        //Build
        VkAccelerationStructureGeometryKHR accelerationStructureGeometry{};
        accelerationStructureGeometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
        accelerationStructureGeometry.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
        accelerationStructureGeometry.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
        accelerationStructureGeometry.geometry.triangles.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
        accelerationStructureGeometry.geometry.triangles.vertexFormat = (VkFormat)desc.VertexFormat;
        accelerationStructureGeometry.geometry.triangles.vertexData = vertexBufferDeviceAddress;
        accelerationStructureGeometry.geometry.triangles.maxVertex = 3;
        accelerationStructureGeometry.geometry.triangles.vertexStride = desc.VertexStride;
        accelerationStructureGeometry.geometry.triangles.indexType = VK_INDEX_TYPE_UINT32;
        accelerationStructureGeometry.geometry.triangles.indexData = indexBufferDeviceAddress;

        //Get size info
        VkAccelerationStructureBuildGeometryInfoKHR accelerationStructureBuildGeometryInfo{};
        accelerationStructureBuildGeometryInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
        accelerationStructureBuildGeometryInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
        accelerationStructureBuildGeometryInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
        accelerationStructureBuildGeometryInfo.geometryCount = 1;
        accelerationStructureBuildGeometryInfo.pGeometries = &accelerationStructureGeometry;

        const uint32_t numTriangles = desc.NumTriangles;
        VkAccelerationStructureBuildSizesInfoKHR accelerationStructureBuildSizesInfo{};
        accelerationStructureBuildSizesInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
        vkGetAccelerationStructureBuildSizesKHR(
            (VkDevice)m_LogicalDevice,
            VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
            &accelerationStructureBuildGeometryInfo,
            &numTriangles,
            &accelerationStructureBuildSizesInfo);


        VkBuffer blasBuffer;
        VmaAllocation blasAllocation;
        {
            VkBufferCreateInfo bufferInfo{};
            bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
            bufferInfo.size = accelerationStructureBuildSizesInfo.accelerationStructureSize;
            bufferInfo.usage = VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
            bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

            VmaAllocationCreateInfo allocInfo = {};
            allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
            allocInfo.requiredFlags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT_KHR;

            VK_CHECK(vmaCreateBuffer((VmaAllocator)m_Allocator, &bufferInfo, &allocInfo,
                &blasBuffer,
                &blasAllocation,
                nullptr));
        }

        VkAccelerationStructureCreateInfoKHR accelerationStructureCreateInfo{};
        accelerationStructureCreateInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
        accelerationStructureCreateInfo.buffer = blasBuffer;
        accelerationStructureCreateInfo.size = accelerationStructureBuildSizesInfo.accelerationStructureSize;
        accelerationStructureCreateInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
        VkAccelerationStructureKHR blasHandle;
        vkCreateAccelerationStructureKHR((VkDevice)m_LogicalDevice, &accelerationStructureCreateInfo, nullptr, &blasHandle);

        VkBuffer scratchBuffer;
        VmaAllocation scratchBufferAllocation;
        {
            VkBufferCreateInfo bufferInfo{};
            bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
            bufferInfo.size = accelerationStructureBuildSizesInfo.buildScratchSize;
            bufferInfo.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
            bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

            VmaAllocationCreateInfo allocInfo = {};
            allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
            allocInfo.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

            VK_CHECK(vmaCreateBuffer((VmaAllocator)m_Allocator, &bufferInfo, &allocInfo,
                &scratchBuffer,
                &scratchBufferAllocation,
                nullptr));
        }
        addrInfo.buffer = scratchBuffer;
        VkDeviceAddress scratchBufferDeviceAddress = vkGetBufferDeviceAddress((VkDevice)m_LogicalDevice, &addrInfo);

        VkAccelerationStructureBuildGeometryInfoKHR accelerationBuildGeometryInfo{};
        accelerationBuildGeometryInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
        accelerationBuildGeometryInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
        accelerationBuildGeometryInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
        accelerationBuildGeometryInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
        accelerationBuildGeometryInfo.dstAccelerationStructure = blasHandle;
        accelerationBuildGeometryInfo.geometryCount = 1;
        accelerationBuildGeometryInfo.pGeometries = &accelerationStructureGeometry;
        accelerationBuildGeometryInfo.scratchData.deviceAddress = scratchBufferDeviceAddress;

        VkAccelerationStructureBuildRangeInfoKHR accelerationStructureBuildRangeInfo{};
        accelerationStructureBuildRangeInfo.primitiveCount = numTriangles;
        accelerationStructureBuildRangeInfo.primitiveOffset = 0;
        accelerationStructureBuildRangeInfo.firstVertex = 0;
        accelerationStructureBuildRangeInfo.transformOffset = 0;
        std::vector<VkAccelerationStructureBuildRangeInfoKHR*> accelerationBuildStructureRangeInfos = { &accelerationStructureBuildRangeInfo };

        m_Device->ImmediateSubmit([=](CommandBufferHandle cmd) {
            vkCmdBuildAccelerationStructuresKHR(
                (VkCommandBuffer)cmd,
                1,
                &accelerationBuildGeometryInfo,
                accelerationBuildStructureRangeInfos.data());
            });

        vmaDestroyBuffer((VmaAllocator)m_Allocator, (VkBuffer)scratchBuffer, (VmaAllocation)scratchBufferAllocation);


        VkAccelerationStructureDeviceAddressInfoKHR accelerationDeviceAddressInfo{};
        accelerationDeviceAddressInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
        accelerationDeviceAddressInfo.accelerationStructure = blasHandle;
        VkDeviceAddress blasDeviceAddress = vkGetAccelerationStructureDeviceAddressKHR((VkDevice)m_LogicalDevice, &accelerationDeviceAddressInfo);
        bottomLevelAS->m_Handle = blasHandle;
        bottomLevelAS->m_Buffer = blasBuffer;
        bottomLevelAS->m_Allocation = blasAllocation;
        bottomLevelAS->m_DeviceAddress = blasDeviceAddress;

        m_Resources[bottomLevelAS] = ResourceType::BottomLevelAS;
    }

    void ResourceCache::ReleaseResource(BottomLevelAS* bottomLevelAS)
    {
        if (!m_Resources.contains(bottomLevelAS))
        {
            ARC_LOG_FATAL("Cannot find BottomLevelAS resource to release!");
        }

        vkDestroyAccelerationStructureKHR((VkDevice)m_LogicalDevice, (VkAccelerationStructureKHR)bottomLevelAS->m_Handle, nullptr);
        vmaDestroyBuffer((VmaAllocator)m_Allocator, (VkBuffer)bottomLevelAS->m_Buffer, (VmaAllocation)bottomLevelAS->m_Allocation);

        m_Resources.erase(bottomLevelAS);
    }
}