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

    void TopLevelAS::AddInstance(const TopLevelASInstance& instance)
    {
        m_Instances.push_back(instance);
    }

    void TopLevelAS::ClearInstances()
    {
        m_Instances.clear();
    }

    void TopLevelAS::Build()
    {
        std::vector<VkAccelerationStructureInstanceKHR> instances;
        std::vector<VkTransformMatrixKHR> matrices;
        instances.resize(m_Instances.size());
        matrices.resize(m_Instances.size());
        for (size_t i = 0; i < m_Instances.size(); i++)
        {
            VkTransformMatrixKHR& transformMatrix = matrices[i];
            memcpy(transformMatrix.matrix, m_Instances[i].TransformMatrix, sizeof(float) * 12);

            VkAccelerationStructureInstanceKHR instance{};
            instance.transform = transformMatrix;
            instance.instanceCustomIndex = m_Instances[i].InstanceCustomIndex;
            instance.mask = 0xFF;
            instance.instanceShaderBindingTableRecordOffset = 0;
            instance.flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR;
            instance.accelerationStructureReference = m_Instances[i].BottomLevelASHandle;
            instances[i] = instance;
        }

        VkBuffer instanceBuffer;
        VmaAllocation instanceBufferAllocation;
        {
            VkBufferCreateInfo bufferInfo{};
            bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
            bufferInfo.size = sizeof(VkAccelerationStructureInstanceKHR) * instances.size();
            bufferInfo.usage = VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR;
            bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

            VmaAllocationCreateInfo allocInfo = {};
            allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
            allocInfo.requiredFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
            allocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;

            VK_CHECK(vmaCreateBuffer((VmaAllocator)m_Allocator, &bufferInfo, &allocInfo,
                &instanceBuffer,
                &instanceBufferAllocation,
                nullptr));

            void* data;
            vmaMapMemory((VmaAllocator)m_Allocator, instanceBufferAllocation, &data);
            memcpy(data, instances.data(), instances.size() * sizeof(VkAccelerationStructureInstanceKHR));
            vmaUnmapMemory((VmaAllocator)m_Allocator, instanceBufferAllocation);
        }

        m_InstanceBuffer = instanceBuffer;
        m_InstanceAllocation = instanceBufferAllocation;

        VkBufferDeviceAddressInfo addrInfo{ VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO };
        addrInfo.buffer = instanceBuffer;
        VkDeviceAddress instanceBufferDeviceAddress = vkGetBufferDeviceAddress((VkDevice)m_LogicalDevice, &addrInfo);

        VkDeviceOrHostAddressConstKHR instanceDataDeviceAddress{};
        instanceDataDeviceAddress.deviceAddress = instanceBufferDeviceAddress;

        VkAccelerationStructureGeometryKHR accelerationStructure2Geometry{};
        accelerationStructure2Geometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
        accelerationStructure2Geometry.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
        accelerationStructure2Geometry.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
        accelerationStructure2Geometry.geometry.instances.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
        accelerationStructure2Geometry.geometry.instances.arrayOfPointers = VK_FALSE;
        accelerationStructure2Geometry.geometry.instances.data = instanceDataDeviceAddress;

        // Get size info
        /*
        The pSrcAccelerationStructure, dstAccelerationStructure, and mode members of pBuildInfo are ignored. Any VkDeviceOrHostAddressKHR members of pBuildInfo are ignored by this command, except that the hostAddress member of VkAccelerationStructureGeometryTrianglesDataKHR::transformData will be examined to check if it is NULL.*
        */
        VkAccelerationStructureBuildGeometryInfoKHR accelerationStructureBuild2GeometryInfo{};
        accelerationStructureBuild2GeometryInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
        accelerationStructureBuild2GeometryInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
        accelerationStructureBuild2GeometryInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
        accelerationStructureBuild2GeometryInfo.geometryCount = 1;
        accelerationStructureBuild2GeometryInfo.pGeometries = &accelerationStructure2Geometry;

        uint32_t primitive_count = static_cast<uint32_t>(instances.size());

        VkAccelerationStructureBuildSizesInfoKHR tlasaccelerationStructureBuildSizesInfo{};
        tlasaccelerationStructureBuildSizesInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
        vkGetAccelerationStructureBuildSizesKHR(
            (VkDevice)m_LogicalDevice,
            VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
            &accelerationStructureBuild2GeometryInfo,
            &primitive_count,
            &tlasaccelerationStructureBuildSizesInfo);

        VkBuffer tlasBuffer;
        VmaAllocation tlasAllocation;
        {
            VkBufferCreateInfo bufferInfo{};
            bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
            bufferInfo.size = tlasaccelerationStructureBuildSizesInfo.accelerationStructureSize;
            bufferInfo.usage = VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
            bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

            VmaAllocationCreateInfo allocInfo = {};
            allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
            allocInfo.requiredFlags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT_KHR;

            VK_CHECK(vmaCreateBuffer((VmaAllocator)m_Allocator, &bufferInfo, &allocInfo,
                &tlasBuffer,
                &tlasAllocation,
                nullptr));
        }


        VkAccelerationStructureCreateInfoKHR tlasaccelerationStructureCreateInfo{};
        tlasaccelerationStructureCreateInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
        tlasaccelerationStructureCreateInfo.buffer = tlasBuffer;
        tlasaccelerationStructureCreateInfo.size = tlasaccelerationStructureBuildSizesInfo.accelerationStructureSize;
        tlasaccelerationStructureCreateInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;

        VkAccelerationStructureKHR tlas;
        vkCreateAccelerationStructureKHR((VkDevice)m_LogicalDevice, &tlasaccelerationStructureCreateInfo, nullptr, &tlas);


        VkBuffer scratchBuffer2;
        VmaAllocation scratchBuffer2Allocation;
        {
            VkBufferCreateInfo bufferInfo{};
            bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
            bufferInfo.size = tlasaccelerationStructureBuildSizesInfo.buildScratchSize;
            bufferInfo.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
            bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

            VmaAllocationCreateInfo allocInfo = {};
            allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
            allocInfo.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

            VK_CHECK(vmaCreateBuffer((VmaAllocator)m_Allocator, &bufferInfo, &allocInfo,
                &scratchBuffer2,
                &scratchBuffer2Allocation,
                nullptr));
        }
        addrInfo.buffer = scratchBuffer2;
        VkDeviceAddress scratchBuffer2DeviceAddress = vkGetBufferDeviceAddress((VkDevice)m_LogicalDevice, &addrInfo);


        VkAccelerationStructureBuildGeometryInfoKHR accelerationBuild2GeometryInfo{};
        accelerationBuild2GeometryInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
        accelerationBuild2GeometryInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
        accelerationBuild2GeometryInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
        accelerationBuild2GeometryInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
        accelerationBuild2GeometryInfo.dstAccelerationStructure = tlas;
        accelerationBuild2GeometryInfo.geometryCount = 1;
        accelerationBuild2GeometryInfo.pGeometries = &accelerationStructure2Geometry;
        accelerationBuild2GeometryInfo.scratchData.deviceAddress = scratchBuffer2DeviceAddress;

        VkAccelerationStructureBuildRangeInfoKHR accelerationStructureBuild2RangeInfo{};
        accelerationStructureBuild2RangeInfo.primitiveCount = primitive_count;
        accelerationStructureBuild2RangeInfo.primitiveOffset = 0;
        accelerationStructureBuild2RangeInfo.firstVertex = 0;
        accelerationStructureBuild2RangeInfo.transformOffset = 0;
        std::vector<VkAccelerationStructureBuildRangeInfoKHR*> accelerationBuild2StructureRangeInfos = { &accelerationStructureBuild2RangeInfo };

        m_Device->ImmediateSubmit([=](CommandBufferHandle cmd) {
            vkCmdBuildAccelerationStructuresKHR(
                (VkCommandBuffer)cmd,
                1,
                &accelerationBuild2GeometryInfo,
                accelerationBuild2StructureRangeInfos.data());
            });

        vmaDestroyBuffer((VmaAllocator)m_Allocator, (VkBuffer)scratchBuffer2, (VmaAllocation)scratchBuffer2Allocation);

        m_Handle = tlas;
        m_Buffer = tlasBuffer;
        m_Allocation = tlasAllocation;
    }

    void ResourceCache::CreateTopLevelAS(TopLevelAS* topLevelAS, const TopLevelASDesc& desc)
    {
        topLevelAS->m_Device = m_Device;
        topLevelAS->m_LogicalDevice = m_LogicalDevice;
        topLevelAS->m_Allocator = m_Allocator;

        m_Resources[topLevelAS] = ResourceType::TopLevelAS;
    }

    void ResourceCache::ReleaseResource(TopLevelAS* topLevelAS)
    {
        if (!m_Resources.contains(topLevelAS))
        {
            ARC_LOG_FATAL("Cannot find TopLevelAS resource to release!");
        }

        vkDestroyAccelerationStructureKHR((VkDevice)m_LogicalDevice, (VkAccelerationStructureKHR)topLevelAS->m_Handle, nullptr);
        vmaDestroyBuffer((VmaAllocator)m_Allocator, (VkBuffer)topLevelAS->m_Buffer, (VmaAllocation)topLevelAS->m_Allocation);
        vmaDestroyBuffer((VmaAllocator)m_Allocator, (VkBuffer)topLevelAS->m_InstanceBuffer, (VmaAllocation)topLevelAS->m_InstanceAllocation);

        m_Resources.erase(topLevelAS);
    }
}