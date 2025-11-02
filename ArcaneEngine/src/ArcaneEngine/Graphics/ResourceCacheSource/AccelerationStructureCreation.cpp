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


    void ResourceCache::CreateAccelerationStructure(AccelerationStructure* accelerationStructure, const AccelerationStructureDesc& desc)
    {      
        VkDeviceOrHostAddressConstKHR vertexBufferDeviceAddress{};
        VkDeviceOrHostAddressConstKHR indexBufferDeviceAddress{};

        VkBufferDeviceAddressInfo addrInfo{ VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO };
        addrInfo.buffer = (VkBuffer)desc.BLAccelerationStructure.VertexBuffer;
        vertexBufferDeviceAddress.deviceAddress = vkGetBufferDeviceAddress((VkDevice)m_LogicalDevice, &addrInfo);
        addrInfo.buffer = (VkBuffer)desc.BLAccelerationStructure.IndexBuffer;
        indexBufferDeviceAddress.deviceAddress = vkGetBufferDeviceAddress((VkDevice)m_LogicalDevice, &addrInfo);

        //Build
        VkAccelerationStructureGeometryKHR accelerationStructureGeometry{};
        accelerationStructureGeometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
        accelerationStructureGeometry.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
        accelerationStructureGeometry.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
        accelerationStructureGeometry.geometry.triangles.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
        accelerationStructureGeometry.geometry.triangles.vertexFormat = (VkFormat)desc.BLAccelerationStructure.Format;
        accelerationStructureGeometry.geometry.triangles.vertexData = vertexBufferDeviceAddress;
        accelerationStructureGeometry.geometry.triangles.maxVertex = 3;
        accelerationStructureGeometry.geometry.triangles.vertexStride = desc.BLAccelerationStructure.VertexStride;
        accelerationStructureGeometry.geometry.triangles.indexType = VK_INDEX_TYPE_UINT32;
        accelerationStructureGeometry.geometry.triangles.indexData = indexBufferDeviceAddress;

        //Get size info
        VkAccelerationStructureBuildGeometryInfoKHR accelerationStructureBuildGeometryInfo{};
        accelerationStructureBuildGeometryInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
        accelerationStructureBuildGeometryInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
        accelerationStructureBuildGeometryInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
        accelerationStructureBuildGeometryInfo.geometryCount = 1;
        accelerationStructureBuildGeometryInfo.pGeometries = &accelerationStructureGeometry;

        const uint32_t numTriangles = desc.BLAccelerationStructure.NumOfTriangles;
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
        VkAccelerationStructureKHR blas;
        vkCreateAccelerationStructureKHR((VkDevice)m_LogicalDevice, &accelerationStructureCreateInfo, nullptr, &blas);
        accelerationStructure->m_BLAccelerationStructure.Handle = blas;
        accelerationStructure->m_BLAccelerationStructure.Buffer = blasBuffer;
        accelerationStructure->m_BLAccelerationStructure.Allocation = blasAllocation;

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
        accelerationBuildGeometryInfo.dstAccelerationStructure = blas;
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
        accelerationDeviceAddressInfo.accelerationStructure = blas;        
        VkDeviceAddress blasDeviceAddress = vkGetAccelerationStructureDeviceAddressKHR((VkDevice)m_LogicalDevice, &accelerationDeviceAddressInfo);



        // TOP LEVEL ACCELERATION STRUCTURE

        VkTransformMatrixKHR transformMatrix2 = {
            1.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 1.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 1.0f, 0.0f };

        VkAccelerationStructureInstanceKHR instance{};
        instance.transform = transformMatrix2;
        instance.instanceCustomIndex = 0;
        instance.mask = 0xFF;
        instance.instanceShaderBindingTableRecordOffset = 0;
        instance.flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR;
        instance.accelerationStructureReference = blasDeviceAddress;

        VkBuffer instanceBuffer;
        VmaAllocation instanceBufferAllocation;
        {
            VkBufferCreateInfo bufferInfo{};
            bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
            bufferInfo.size = sizeof(VkAccelerationStructureInstanceKHR);
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
            memcpy(data, &instance, sizeof(VkAccelerationStructureInstanceKHR));
            vmaUnmapMemory((VmaAllocator)m_Allocator, instanceBufferAllocation);
        }

        accelerationStructure->m_InstanceBuffer = instanceBuffer;
        accelerationStructure->m_InstanceAllocation = instanceBufferAllocation;

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

        uint32_t primitive_count = 1;

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
        accelerationStructure->m_TLAccelerationStructure.Handle = tlas;
        accelerationStructure->m_TLAccelerationStructure.Buffer = tlasBuffer;
        accelerationStructure->m_TLAccelerationStructure.Allocation = tlasAllocation;


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
        accelerationStructureBuild2RangeInfo.primitiveCount = 1;
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

        VkAccelerationStructureDeviceAddressInfoKHR accelerationDevice2AddressInfo{};
        accelerationDevice2AddressInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
        accelerationDevice2AddressInfo.accelerationStructure = tlas;
        VkDeviceAddress tlasDeviceAddress = vkGetAccelerationStructureDeviceAddressKHR((VkDevice)m_LogicalDevice, &accelerationDevice2AddressInfo);

        //instancesBuffer.destroy();


        m_Resources[accelerationStructure] = ResourceType::AccelerationStructure;        
    }

    void CreateAccelerationStructureBuffer(VkBuffer& buffer, VmaAllocation& allocation, VkDeviceSize size, VmaAllocator allocator)
    {
        {
            VkBufferCreateInfo bufferInfo{};
            bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
            bufferInfo.size = size;
            bufferInfo.usage = VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
            bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

            VmaAllocationCreateInfo allocInfo = {};
            allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
            allocInfo.requiredFlags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT_KHR;

            VK_CHECK(vmaCreateBuffer(allocator, &bufferInfo, &allocInfo,
                &buffer,
                &allocation,
                nullptr));
        }
    }

    void ResourceCache::ReleaseResource(AccelerationStructure* accelerationStructure)
    {
        if (!m_Resources.contains(accelerationStructure))
        {
            ARC_LOG_FATAL("Cannot find AccelerationStructure resource to release!");
        }

        vkDestroyAccelerationStructureKHR((VkDevice)m_LogicalDevice, (VkAccelerationStructureKHR)accelerationStructure->m_TLAccelerationStructure.Handle, nullptr);
        vkDestroyAccelerationStructureKHR((VkDevice)m_LogicalDevice, (VkAccelerationStructureKHR)accelerationStructure->m_BLAccelerationStructure.Handle, nullptr);
        vmaDestroyBuffer((VmaAllocator)m_Allocator, (VkBuffer)accelerationStructure->m_TLAccelerationStructure.Buffer, (VmaAllocation)accelerationStructure->m_TLAccelerationStructure.Allocation);
        vmaDestroyBuffer((VmaAllocator)m_Allocator, (VkBuffer)accelerationStructure->m_BLAccelerationStructure.Buffer, (VmaAllocation)accelerationStructure->m_BLAccelerationStructure.Allocation);
        vmaDestroyBuffer((VmaAllocator)m_Allocator, (VkBuffer)accelerationStructure->m_InstanceBuffer, (VmaAllocation)accelerationStructure->m_InstanceAllocation);

        m_Resources.erase(accelerationStructure);
    }
}