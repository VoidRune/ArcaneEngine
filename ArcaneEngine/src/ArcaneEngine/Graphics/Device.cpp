#include "Device.h"
#include "VulkanCore/VulkanHandleCreation.h"
#include "VulkanCore/VulkanLocal.h"
#include "ArcaneEngine/Core/Log.h"
#include <vulkan/vulkan_core.h>
#include <vector>

namespace Arc
{

	Device::Device(void* windowHandle, const std::vector<const char*>& instanceExtensions, uint32_t framesInFlight)
	{
        CreateInstance(instanceExtensions);
        CreateDebugUtilsMessenger();
        SelectPhysicalDevice();
        CreateSurface(windowHandle, framesInFlight);
        CreateLogicalDevice();

        m_ResourceCache = std::make_unique<ResourceCache>(this);
        m_RenderGraph = std::make_unique<RenderGraph>();
	}

	Device::~Device()
	{
        m_ResourceCache.reset();
        m_RenderGraph.reset();

        VK_CHECK(vkDeviceWaitIdle((VkDevice)m_LogicalDevice));
        vkDestroyCommandPool((VkDevice)m_LogicalDevice, (VkCommandPool)m_CommandPool, nullptr);
        vkDestroyDevice((VkDevice)m_LogicalDevice, nullptr);
        vkDestroySurfaceKHR((VkInstance)m_Instance, (VkSurfaceKHR)m_Surface, nullptr);
#ifdef ARCANE_ENABLE_VALIDATION
        PFN_vkDestroyDebugUtilsMessengerEXT func =
            (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr((VkInstance)m_Instance, "vkDestroyDebugUtilsMessengerEXT");
        func((VkInstance)m_Instance, (VkDebugUtilsMessengerEXT)m_DebugUtilsMessenger, nullptr);
#endif
        vkDestroyInstance((VkInstance)m_Instance, nullptr);
	}

    void Device::WaitIdle()
    {
        vkDeviceWaitIdle((VkDevice)m_LogicalDevice);
    }

    void Device::UpdateDescriptorSet(DescriptorSet* descriptor, const DescriptorWrite& write)
    {
        std::vector<VkWriteDescriptorSet> writeDescriptorSet;
        writeDescriptorSet.reserve(write.m_BufferWrites.size() + write.m_ImageWrites.size());

        if (write.m_BufferArrayWrites.size() > 0)
        {
            ARC_LOG_WARNING("UpdateDescriptorSet write contains BufferArrayWrites, consider using DescriptorSetArray!");
        }

        std::vector<VkDescriptorBufferInfo> bufferInfos;
        bufferInfos.reserve(write.m_BufferWrites.size());

        for (auto& bw : write.m_BufferWrites)
        {
            VkDescriptorBufferInfo bufferInfo = {};
            bufferInfo.buffer = (VkBuffer)bw.Buffer->GetHandle();
            bufferInfo.offset = 0;
            bufferInfo.range = bw.Buffer->GetSize();
            bufferInfos.push_back(bufferInfo);

            VkWriteDescriptorSet writeInfo = {};
            writeInfo.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            writeInfo.dstSet = (VkDescriptorSet)descriptor->GetHandle();
            writeInfo.dstBinding = bw.Binding;
            writeInfo.dstArrayElement = 0;
            writeInfo.descriptorType = (VkDescriptorType)descriptor->GetBindingType(bw.Binding);
            writeInfo.descriptorCount = 1;
            writeInfo.pBufferInfo = &bufferInfos[bufferInfos.size() - 1];
            writeDescriptorSet.push_back(writeInfo);
        }

        std::vector<VkDescriptorImageInfo> imageInfos;
        imageInfos.reserve(write.m_ImageWrites.size());
  
        for (auto& iw : write.m_ImageWrites)
        {
            VkDescriptorImageInfo imageInfo = {};
            if (iw.Image)
                imageInfo.imageView = (VkImageView)iw.Image->GetImageView();
            imageInfo.imageLayout = (VkImageLayout)iw.ImageLayout;
            if(iw.Sampler)
                imageInfo.sampler = (VkSampler)iw.Sampler->GetHandle();
            imageInfos.push_back(imageInfo);

            VkWriteDescriptorSet writeInfo = {};
            writeInfo.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            writeInfo.dstSet = (VkDescriptorSet)descriptor->GetHandle();
            writeInfo.dstBinding = iw.Binding;
            writeInfo.dstArrayElement = 0;
            writeInfo.descriptorType = (VkDescriptorType)descriptor->GetBindingType(iw.Binding);
            writeInfo.descriptorCount = 1;
            writeInfo.pImageInfo = &imageInfos[imageInfos.size() - 1];
            writeDescriptorSet.push_back(writeInfo);
        }

        vkUpdateDescriptorSets((VkDevice)m_LogicalDevice, static_cast<uint32_t>(writeDescriptorSet.size()), writeDescriptorSet.data(), 0, NULL);
    }

    void Device::UpdateDescriptorSet(DescriptorSetArray* descriptorArray, const DescriptorWrite& write)
    {
        auto descriptorSets = descriptorArray->GetHandles();

        std::vector<VkWriteDescriptorSet> writeDescriptorSet;
        writeDescriptorSet.reserve(descriptorSets.size() * (write.m_BufferArrayWrites.size() + write.m_BufferWrites.size() + write.m_ImageWrites.size()));

        std::vector<VkDescriptorBufferInfo> bufferInfos;
        bufferInfos.reserve(descriptorSets.size() * (write.m_BufferArrayWrites.size() + write.m_BufferWrites.size()));

        std::vector<VkDescriptorImageInfo> imageInfos;
        imageInfos.reserve(descriptorSets.size() * write.m_ImageWrites.size());

        for (int i = 0; i < descriptorSets.size(); i++)
        {
            auto descriptor = descriptorSets[i];

            for (auto& baw : write.m_BufferArrayWrites)
            {
                VkDescriptorBufferInfo bufferInfo = {};
                bufferInfo.buffer = (VkBuffer)baw.Buffer->GetHandle(i);
                bufferInfo.offset = 0;
                bufferInfo.range = baw.Buffer->GetSize();
                bufferInfos.push_back(bufferInfo);

                VkWriteDescriptorSet writeInfo = {};
                writeInfo.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                writeInfo.dstSet = (VkDescriptorSet)descriptor;
                writeInfo.dstBinding = baw.Binding;
                writeInfo.dstArrayElement = 0;
                writeInfo.descriptorType = (VkDescriptorType)descriptorArray->GetBindingType(baw.Binding);
                writeInfo.descriptorCount = 1;
                writeInfo.pBufferInfo = &bufferInfos[bufferInfos.size() - 1];
                writeDescriptorSet.push_back(writeInfo);
            }
            for (auto& bw : write.m_BufferWrites)
            {
                VkDescriptorBufferInfo bufferInfo = {};
                bufferInfo.buffer = (VkBuffer)bw.Buffer->GetHandle();
                bufferInfo.offset = 0;
                bufferInfo.range = bw.Buffer->GetSize();
                bufferInfos.push_back(bufferInfo);

                VkWriteDescriptorSet writeInfo = {};
                writeInfo.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                writeInfo.dstSet = (VkDescriptorSet)descriptor;
                writeInfo.dstBinding = bw.Binding;
                writeInfo.dstArrayElement = 0;
                writeInfo.descriptorType = (VkDescriptorType)descriptorArray->GetBindingType(bw.Binding);
                writeInfo.descriptorCount = 1;
                writeInfo.pBufferInfo = &bufferInfos[bufferInfos.size() - 1];
                writeDescriptorSet.push_back(writeInfo);
            }
            for (auto& iw : write.m_ImageWrites)
            {
                VkDescriptorImageInfo imageInfo = {};
                if (iw.Image)
                    imageInfo.imageView = (VkImageView)iw.Image->GetImageView();
                imageInfo.imageLayout = (VkImageLayout)iw.ImageLayout;
                if (iw.Sampler)
                    imageInfo.sampler = (VkSampler)iw.Sampler->GetHandle();
                imageInfos.push_back(imageInfo);

                VkWriteDescriptorSet writeInfo = {};
                writeInfo.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                writeInfo.dstSet = (VkDescriptorSet)descriptor;
                writeInfo.dstBinding = iw.Binding;
                writeInfo.dstArrayElement = 0;
                writeInfo.descriptorType = (VkDescriptorType)descriptorArray->GetBindingType(iw.Binding);
                writeInfo.descriptorCount = 1;
                writeInfo.pImageInfo = &imageInfos[imageInfos.size() - 1];
                writeDescriptorSet.push_back(writeInfo);
            }
        }

        vkUpdateDescriptorSets((VkDevice)m_LogicalDevice, static_cast<uint32_t>(writeDescriptorSet.size()), writeDescriptorSet.data(), 0, NULL);
    }
    
    void Device::CreateInstance(const std::vector<const char*>& instanceExtensions)
    {
        InstanceCreateInfo instanceCreateInfo = {
            .applicationName = "Application",
            .engineName = "Arcane Renderer",
            .apiVersion = ApiVersion(1, 3),
            .instanceExtensions = instanceExtensions,
#ifdef ARCANE_ENABLE_VALIDATION
            .enableValidationLayers = true
#endif
        };

        m_Instance = CreateInstanceHandle(instanceCreateInfo);
    }

    void Device::CreateDebugUtilsMessenger()
    {
        DebugUtilsMessengerCreateInfo debugUtilsMessengerInfo = {
            .instance = m_Instance,
#ifdef ARCANE_ENABLE_VALIDATION
            .enableDebugUtilsMessenger = true
#endif
        };

        m_DebugUtilsMessenger = CreateDebugUtilsMessengerHandle(debugUtilsMessengerInfo);
    }

    void Device::SelectPhysicalDevice()
    {
        PhysicalDeviceSelectInfo physicalDeviceSelectInfo = {
            .instance = m_Instance,
        };

        m_PhysicalDevice = SelectPhysicalDeviceHandle(physicalDeviceSelectInfo);
    }

    void Device::CreateSurface(void* windowHandle, uint32_t framesInFlight)
    {
        SurfaceCreateInfo surfaceCreateInfo = {
            .instance = m_Instance,
            .windowHandle = windowHandle
        };

        m_Surface = CreateSurfaceHandle(surfaceCreateInfo);

        VkSurfaceCapabilitiesKHR capabilities;
        VK_CHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR((VkPhysicalDevice)m_PhysicalDevice, (VkSurfaceKHR)m_Surface, &capabilities));

        uint32_t framesCount = std::max(framesInFlight, capabilities.minImageCount);
        if (capabilities.maxImageCount > 0 && framesCount > capabilities.maxImageCount)
            framesCount = capabilities.maxImageCount;

        m_FramesInFlight = framesCount;
    }

    void Device::CreateLogicalDevice()
    {
        QueueFamilySelectInfo queueFamilySelectInfo = {
            .physicalDevice = m_PhysicalDevice,
            .surface = m_Surface
        };

        m_QueueFamiliyIndices = SelectQueueFamilies(queueFamilySelectInfo);

        DeviceCreateInfo deviceCreateInfo = {
            .physicalDevice = m_PhysicalDevice,
            .queueFamilyIndices = m_QueueFamiliyIndices
        };

        m_LogicalDevice = CreateLogicalDeviceHandle(deviceCreateInfo);

        /* Queues */
        VkQueue graphicsQueue;
        VkQueue presentQueue;

        vkGetDeviceQueue((VkDevice)m_LogicalDevice, m_QueueFamiliyIndices.GraphicsIndex, 0, &graphicsQueue);
        vkGetDeviceQueue((VkDevice)m_LogicalDevice, m_QueueFamiliyIndices.PresentIndex, 0, &presentQueue);

        m_GraphicsQueue = graphicsQueue;
        m_PresentQueue = presentQueue;

        /* Command pools */
        VkCommandPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        poolInfo.queueFamilyIndex = m_QueueFamiliyIndices.GraphicsIndex;

        VkCommandPool commandPool;
        VK_CHECK(vkCreateCommandPool((VkDevice)m_LogicalDevice, &poolInfo, nullptr, &commandPool));
        m_CommandPool = commandPool;
    }
}