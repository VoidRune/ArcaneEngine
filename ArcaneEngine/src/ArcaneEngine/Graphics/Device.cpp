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

    void Device::ImmediateSubmit(std::function<void(CommandBufferHandle cmd)>&& func)
    {
        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandPool = (VkCommandPool)m_CommandPool;
        allocInfo.commandBufferCount = 1;

        VkCommandBuffer commandBuffer;
        VK_CHECK(vkAllocateCommandBuffers((VkDevice)m_LogicalDevice, &allocInfo, &commandBuffer));

        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        VK_CHECK(vkBeginCommandBuffer(commandBuffer, &beginInfo));

        func(commandBuffer);

        VK_CHECK(vkEndCommandBuffer(commandBuffer));

        VkFence uploadFence;
        VkFenceCreateInfo uploadFenceCreateInfo{};
        uploadFenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        uploadFenceCreateInfo.pNext = nullptr;
        uploadFenceCreateInfo.flags = 0;
        VK_CHECK(vkCreateFence((VkDevice)m_LogicalDevice, &uploadFenceCreateInfo, nullptr, &uploadFence));

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffer;

        VK_CHECK(vkQueueSubmit((VkQueue)m_GraphicsQueue, 1, &submitInfo, uploadFence));

        VK_CHECK(vkWaitForFences((VkDevice)m_LogicalDevice, 1, &uploadFence, true, std::numeric_limits<uint64_t>::max()));
        vkDestroyFence((VkDevice)m_LogicalDevice, uploadFence, nullptr);

        vkFreeCommandBuffers((VkDevice)m_LogicalDevice, (VkCommandPool)m_CommandPool, 1, &commandBuffer);
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

    void Device::TransitionImageLayout(GpuImage* image, ImageLayout newLayout)
    {
        ImmediateSubmit([=](CommandBufferHandle cmd) {
            VkImageLayout tempOldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            VkImageLayout tempNewLayout = static_cast<VkImageLayout>(newLayout);

            VkImageMemoryBarrier barrier{};
            barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            barrier.oldLayout = tempOldLayout;
            barrier.newLayout = tempNewLayout;
            barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.image = (VkImage)image->GetHandle();
            barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            barrier.subresourceRange.baseMipLevel = 0;
            barrier.subresourceRange.levelCount = image->GetMipLevels();
            barrier.subresourceRange.baseArrayLayer = 0;
            barrier.subresourceRange.layerCount = 1;

            barrier.srcAccessMask = 0;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
            VkPipelineStageFlags sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            VkPipelineStageFlags destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;

            if (tempOldLayout == VK_IMAGE_LAYOUT_UNDEFINED && tempNewLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
                barrier.srcAccessMask = 0;
                barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

                sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
                destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
            }
            else if (tempOldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && tempNewLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
                barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

                sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
                destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
            }

            vkCmdPipelineBarrier(
                (VkCommandBuffer)cmd,
                sourceStage, destinationStage,
                0,
                0, nullptr,
                0, nullptr,
                1, &barrier);
        });        
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