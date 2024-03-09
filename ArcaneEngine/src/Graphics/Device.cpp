#include "Device.h"
#include "Core/Log.h"

#include <set>

/* Needed for surface creation */
#define GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

namespace Arc
{

	Device::Device(void* windowHandle, uint32_t imageCount)
	{
        m_ImageCount = imageCount;

        CreateInstance();
        CreateDebugUtilsMessenger();
        FindPhysicalDevice();
        CreateSurface(windowHandle);
        FindQueueFamilyIndices();
        CreateLogicalDevice();
        GetDeviceQueue();
        CreateCommandPool();

        m_GpuProfiler = std::make_unique<GpuProfiler>(m_LogicalDevice, m_PhysicalDevice);
        m_RenderGraph = std::make_unique<RenderGraph>(m_LogicalDevice, m_GpuProfiler.get());
        m_ResourceCache = std::make_unique<ResourceCache>(m_Instance, m_LogicalDevice, m_PhysicalDevice, m_ImageCount);

	}

	Device::~Device()
	{
        m_ResourceCache.reset();
        m_RenderGraph.reset();
        m_GpuProfiler.reset();

        VK_CHECK(vkDeviceWaitIdle(m_LogicalDevice));
        vkDestroyCommandPool(m_LogicalDevice, m_CommandPool, nullptr);
        vkDestroyDevice(m_LogicalDevice, nullptr);
        vkDestroySurfaceKHR(m_Instance, m_Surface, nullptr);

#ifdef ARCANE_ENABLE_VALIDATION
        PFN_vkDestroyDebugUtilsMessengerEXT func =
            (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(m_Instance, "vkDestroyDebugUtilsMessengerEXT");
        func(m_Instance, m_DebugMessenger, nullptr);
#endif

        vkDestroyInstance(m_Instance, nullptr);
	}

    std::unique_ptr<Swapchain> Device::CreateSwapchain(PresentMode preferredMode)
    {
        std::unique_ptr<Swapchain> swapchain = std::unique_ptr<Swapchain>(new Swapchain(m_Instance, m_LogicalDevice, m_PhysicalDevice,
            m_Surface, m_QueueFamilyIndices, m_PresentQueue, m_ImageCount, static_cast<VkPresentModeKHR>(preferredMode)));
        return swapchain;
    }

    void Device::WaitIdle()
    {
        vkDeviceWaitIdle(m_LogicalDevice);
    }

    void Device::ImmediateSubmit(std::function<void(VkCommandBuffer cmd)>&& func)
    {
        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandPool = m_CommandPool;
        allocInfo.commandBufferCount = 1;

        VkCommandBuffer commandBuffer;
        VK_CHECK(vkAllocateCommandBuffers(m_LogicalDevice, &allocInfo, &commandBuffer));

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
        VK_CHECK(vkCreateFence(m_LogicalDevice, &uploadFenceCreateInfo, nullptr, &uploadFence));

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffer;

        VK_CHECK(vkQueueSubmit(m_GraphicsQueue, 1, &submitInfo, uploadFence));

        VK_CHECK(vkWaitForFences(m_LogicalDevice, 1, &uploadFence, true, std::numeric_limits<uint64_t>::max()));
        vkDestroyFence(m_LogicalDevice, uploadFence, nullptr);

        vkFreeCommandBuffers(m_LogicalDevice, m_CommandPool, 1, &commandBuffer);
    }

    void Device::UpdateDescriptorSet(DescriptorSet* descriptorSet, const DescriptorWriteDesc& writeDesc)
    {
        std::vector<VkWriteDescriptorSet> writeDescriptorSet(writeDesc.WriteInfo.size());
        for (int i = 0; i < writeDesc.WriteInfo.size(); i++)
        {
            uint32_t dstBinding = writeDesc.WriteInfo[i].dstBinding;
            writeDescriptorSet[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            writeDescriptorSet[i].dstSet = descriptorSet->GetHandle();
            writeDescriptorSet[i].dstBinding = dstBinding;
            writeDescriptorSet[i].dstArrayElement = writeDesc.WriteInfo[i].dstArrayElement;
            writeDescriptorSet[i].descriptorType = descriptorSet->GetBindingType(dstBinding);
            writeDescriptorSet[i].descriptorCount = writeDesc.WriteInfo[i].descriptorCount;
            writeDescriptorSet[i].pBufferInfo = writeDesc.WriteInfo[i].pBufferInfo;
            writeDescriptorSet[i].pImageInfo = writeDesc.WriteInfo[i].pImageInfo;
            writeDescriptorSet[i].pTexelBufferView = writeDesc.WriteInfo[i].pTexelBufferView;
        }

        vkUpdateDescriptorSets(m_LogicalDevice, static_cast<uint32_t>(writeDescriptorSet.size()), writeDescriptorSet.data(), 0, NULL);
    }

    void Device::UpdateDescriptorSetArray(DescriptorSetArray* descriptorSet, const DescriptorArrayWriteDesc& writeDesc)
    {
        std::vector<VkWriteDescriptorSet> writeDescriptorSet(writeDesc.WriteInfo.size());

        for (int i = 0; i < writeDesc.WriteInfo.size(); i++)
        {
            int frame = i % m_ImageCount;

            writeDescriptorSet[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            writeDescriptorSet[i].dstSet = descriptorSet->GetHandle(frame);
            writeDescriptorSet[i].dstBinding = writeDesc.WriteInfo[i].dstBinding;
            writeDescriptorSet[i].dstArrayElement = writeDesc.WriteInfo[i].dstArrayElement;
            writeDescriptorSet[i].descriptorType = writeDesc.WriteInfo[i].descriptorType;
            writeDescriptorSet[i].descriptorCount = writeDesc.WriteInfo[i].descriptorCount;
            writeDescriptorSet[i].pBufferInfo = writeDesc.WriteInfo[i].pBufferInfo;
            writeDescriptorSet[i].pImageInfo = writeDesc.WriteInfo[i].pImageInfo;
            writeDescriptorSet[i].pTexelBufferView = writeDesc.WriteInfo[i].pTexelBufferView;
        }

        vkUpdateDescriptorSets(m_LogicalDevice, static_cast<uint32_t>(writeDescriptorSet.size()), writeDescriptorSet.data(), 0, NULL);
    }

    void Device::SetImageData(Image* image, const void* data, uint32_t size, ImageLayout newLayout)
    {
        /* Allocate temporary staging buffer */
        VkBufferCreateInfo stagingBufferInfo = {};
        stagingBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        stagingBufferInfo.pNext = nullptr;
        stagingBufferInfo.size = size;
        stagingBufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

        VmaAllocationCreateInfo allocInfo = {};
        allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
        allocInfo.requiredFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
        allocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;

        GpuBuffer stagingBuffer;

        VK_CHECK(vmaCreateBuffer(m_ResourceCache->GetAllocator(), &stagingBufferInfo, &allocInfo,
            &stagingBuffer.m_Buffer,
            &stagingBuffer.m_Allocation,
            nullptr));

        void* copyData;
        VK_CHECK(vmaMapMemory(m_ResourceCache->GetAllocator(), stagingBuffer.m_Allocation, &copyData));
        memcpy(copyData, data, size);
        vmaUnmapMemory(m_ResourceCache->GetAllocator(), stagingBuffer.m_Allocation);

        ImmediateSubmit([=](VkCommandBuffer cmd) {
            VkImageMemoryBarrier barrier{};
            barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.image = image->GetHandle();
            barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            barrier.subresourceRange.baseMipLevel = 0;
            barrier.subresourceRange.levelCount = image->MipLevels();
            barrier.subresourceRange.baseArrayLayer = 0;
            barrier.subresourceRange.layerCount = 1;
            barrier.srcAccessMask = 0;
            barrier.dstAccessMask = 0;

            vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, NULL, 0, NULL, 1, &barrier);

            VkBufferImageCopy region = {};
            region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            region.imageSubresource.layerCount = 1;
            region.imageExtent.width = image->Width();
            region.imageExtent.height = image->Height();
            region.imageExtent.depth = image->Depth();
            vkCmdCopyBufferToImage(cmd, stagingBuffer.m_Buffer, image->GetHandle(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

            VkImageMemoryBarrier use_barrier = {};
            use_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            use_barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            use_barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
            use_barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            use_barrier.newLayout = static_cast<VkImageLayout>(newLayout);
            if (image->MipLevels() > 1)
                use_barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            use_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            use_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            use_barrier.image = image->GetHandle();
            use_barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            use_barrier.subresourceRange.levelCount = 1;
            use_barrier.subresourceRange.layerCount = 1;
            vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, NULL, 0, NULL, 1, &use_barrier);
            });

        /* Release staging buffer */
        vmaDestroyBuffer(m_ResourceCache->GetAllocator(), stagingBuffer.m_Buffer, stagingBuffer.m_Allocation);

        // TODO: Check if image format supports linear blitting
        //VkFormatProperties formatProperties;
        //vkGetPhysicalDeviceFormatProperties(_physicalDevice, image->, &formatProperties);
        //if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT)) {
        //    throw std::runtime_error("texture image format does not support linear blitting!");
        //}

        /* Generate mipmaps */
        if (image->MipLevels() > 1)
        {
            ImmediateSubmit([=](VkCommandBuffer cmd) {
                VkImageMemoryBarrier barrier{};
                barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
                barrier.image = image->GetHandle();
                barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                barrier.subresourceRange.baseArrayLayer = 0;
                barrier.subresourceRange.layerCount = 1;
                barrier.subresourceRange.levelCount = 1;

                int32_t mipWidth = image->Width();
                int32_t mipHeight = image->Height();

                for (uint32_t i = 1; i < image->MipLevels(); i++) {
                    barrier.subresourceRange.baseMipLevel = i - 1;
                    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
                    barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
                    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                    barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

                    vkCmdPipelineBarrier(cmd,
                        VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
                        0, nullptr,
                        0, nullptr,
                        1, &barrier);

                    VkImageBlit blit{};
                    blit.srcOffsets[0] = { 0, 0, 0 };
                    blit.srcOffsets[1] = { mipWidth, mipHeight, 1 };
                    blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                    blit.srcSubresource.mipLevel = i - 1;
                    blit.srcSubresource.baseArrayLayer = 0;
                    blit.srcSubresource.layerCount = 1;
                    blit.dstOffsets[0] = { 0, 0, 0 };
                    blit.dstOffsets[1] = { mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1 };
                    blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                    blit.dstSubresource.mipLevel = i;
                    blit.dstSubresource.baseArrayLayer = 0;
                    blit.dstSubresource.layerCount = 1;

                    vkCmdBlitImage(cmd,
                        image->GetHandle(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                        image->GetHandle(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                        1, &blit,
                        VK_FILTER_LINEAR);

                    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
                    barrier.newLayout = static_cast<VkImageLayout>(newLayout);
                    barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
                    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

                    vkCmdPipelineBarrier(cmd,
                        VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
                        0, nullptr,
                        0, nullptr,
                        1, &barrier);

                    if (mipWidth > 1) mipWidth /= 2;
                    if (mipHeight > 1) mipHeight /= 2;
                }

                barrier.subresourceRange.baseMipLevel = image->MipLevels() - 1;
                barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
                barrier.newLayout = static_cast<VkImageLayout>(newLayout);
                barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

                vkCmdPipelineBarrier(cmd,
                    VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
                    0, nullptr,
                    0, nullptr,
                    1, &barrier);

                });
        }
    }

    void Device::UploadToDeviceLocalBuffer(GpuBuffer* buffer, void* data, uint32_t size)
    {
        /* Allocate staging buffer */
        VkBufferCreateInfo stagingBufferInfo = {};
        stagingBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        stagingBufferInfo.pNext = nullptr;
        stagingBufferInfo.size = size;
        stagingBufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

        VmaAllocationCreateInfo allocInfo = {};
        allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
        allocInfo.requiredFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
        allocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;

        GpuBuffer stagingBuffer;

        VK_CHECK(vmaCreateBuffer(m_ResourceCache->GetAllocator(), &stagingBufferInfo, &allocInfo,
            &stagingBuffer.m_Buffer,
            &stagingBuffer.m_Allocation,
            nullptr));

        void* copyData;
        VK_CHECK(vmaMapMemory(m_ResourceCache->GetAllocator(), stagingBuffer.m_Allocation, &copyData));
        memcpy(copyData, data, size);
        vmaUnmapMemory(m_ResourceCache->GetAllocator(), stagingBuffer.m_Allocation);

        ImmediateSubmit([=](VkCommandBuffer cmd) {
            VkBufferCopy copy;
            copy.dstOffset = 0;
            copy.srcOffset = 0;
            copy.size = size;
            vkCmdCopyBuffer(cmd, stagingBuffer.m_Buffer, buffer->m_Buffer, 1, &copy);
            });

        /* Release staging buffer */
        vmaDestroyBuffer(m_ResourceCache->GetAllocator(), stagingBuffer.m_Buffer, stagingBuffer.m_Allocation);
    }

    void Device::TransitionImageLayout(Image* image, ImageLayout newLayout)
    {
        ImmediateSubmit([=](VkCommandBuffer cmd) {
            VkImageLayout tempOldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            VkImageLayout tempNewLayout = static_cast<VkImageLayout>(newLayout);

            VkImageMemoryBarrier barrier{};
            barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            barrier.oldLayout = tempOldLayout;
            barrier.newLayout = tempNewLayout;
            barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.image = image->GetHandle();
            barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            barrier.subresourceRange.baseMipLevel = 0;
            barrier.subresourceRange.levelCount = image->MipLevels();
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
                cmd,
                sourceStage, destinationStage,
                0,
                0, nullptr,
                0, nullptr,
                1, &barrier);
            });
    }

    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageType,
        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
        void* pUserData);

    void Device::CreateInstance()
    {
        VkApplicationInfo appInfo{};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pNext = nullptr;
        appInfo.pApplicationName = "Application";
        appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.pEngineName = "Vulkan";
        appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.apiVersion = VK_API_VERSION_1_2;

        VkInstanceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInfo.pApplicationInfo = &appInfo;
        createInfo.pNext = nullptr;

        VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};

        uint32_t extensionCount = 0;
        const char** extensions = glfwGetRequiredInstanceExtensions(&extensionCount);
        std::vector<const char*> instanceExtensions(extensions, extensions + extensionCount);
        std::vector<const char*> instanceLayers;

#ifdef ARCANE_ENABLE_VALIDATION
        instanceLayers.push_back("VK_LAYER_KHRONOS_validation");
        instanceExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif
        instanceLayers.push_back("VK_LAYER_KHRONOS_synchronization2");

        createInfo.enabledLayerCount = static_cast<uint32_t>(instanceLayers.size());
        createInfo.ppEnabledLayerNames = instanceLayers.data();

        debugCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        debugCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        debugCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        debugCreateInfo.pfnUserCallback = debugCallback;

        createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
        createInfo.enabledExtensionCount = static_cast<uint32_t>(instanceExtensions.size());
        createInfo.ppEnabledExtensionNames = instanceExtensions.data();

        VK_CHECK(vkCreateInstance(&createInfo, nullptr, &m_Instance));
    }

    void Device::CreateDebugUtilsMessenger()
    {
#ifdef ARCANE_ENABLE_VALIDATION
        VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
        debugCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        debugCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        debugCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        debugCreateInfo.pfnUserCallback = debugCallback;

        PFN_vkCreateDebugUtilsMessengerEXT func =
            (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(m_Instance, "vkCreateDebugUtilsMessengerEXT");
        func(m_Instance, &debugCreateInfo, nullptr, &m_DebugMessenger);
        if (!func)
        {
            VK_CHECK(VK_ERROR_VALIDATION_FAILED_EXT);
        }
#endif
    }

    void Device::FindPhysicalDevice()
    {
        uint32_t deviceCount = 0;
        VK_CHECK(vkEnumeratePhysicalDevices(m_Instance, &deviceCount, nullptr));

        if (deviceCount == 0)
        {
            ARC_LOG("Failed to find GPUs with Vulkan support!");
        }

        std::vector<VkPhysicalDevice> devices(deviceCount);
        VK_CHECK(vkEnumeratePhysicalDevices(m_Instance, &deviceCount, devices.data()));

        int32_t bestScore = 0;
        VkPhysicalDevice bestDevice = VK_NULL_HANDLE;

        for (VkPhysicalDevice& device : devices)
        {
            int32_t score = 0;

            VkPhysicalDeviceProperties deviceProperties;
            vkGetPhysicalDeviceProperties(device, &deviceProperties);

            VkPhysicalDeviceFeatures deviceFeatures;
            vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

            switch (deviceProperties.deviceType)
            {
            case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:      score += 100000; break;
            case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:    score += 1000; break;
            case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:       score += 10; break;
            case VK_PHYSICAL_DEVICE_TYPE_CPU:               score += 1; break;
            }

            if (score > bestScore)
            {
                bestDevice = device;
                bestScore = score;
            }
        }

        if (bestDevice == VK_NULL_HANDLE)
        {
            ARC_LOG("Failed to find suitable GPU!");
        }
        else
        {
            m_PhysicalDevice = bestDevice;
            vkGetPhysicalDeviceProperties(bestDevice, &m_PhysicalDeviceProperties);
            ARC_LOG(std::string("GPU used: ") + m_PhysicalDeviceProperties.deviceName);
        }
    }

    void Device::FindQueueFamilyIndices()
    {
        m_QueueFamilyIndices.graphicsFamilyIndex = uint32_t(-1);
        m_QueueFamilyIndices.presentFamilyIndex = uint32_t(-1);

        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(m_PhysicalDevice, &queueFamilyCount, nullptr);

        std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(m_PhysicalDevice, &queueFamilyCount, queueFamilies.data());

        int i = 0;
        for (VkQueueFamilyProperties& queueFamily : queueFamilies)
        {
            if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT && queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT) {
                m_QueueFamilyIndices.graphicsFamilyIndex = i;
            }

            VkBool32 presentSupport = false;
            VK_CHECK(vkGetPhysicalDeviceSurfaceSupportKHR(m_PhysicalDevice, i, m_Surface, &presentSupport));
            if (presentSupport) {
                m_QueueFamilyIndices.presentFamilyIndex = i;
            }

            if (m_QueueFamilyIndices.graphicsFamilyIndex != -1 &&
                m_QueueFamilyIndices.presentFamilyIndex != -1) {
                break;
            }

            i++;
        }

        if (m_QueueFamilyIndices.graphicsFamilyIndex == uint32_t(-1))
        {
            ARC_LOG("Failed to find appropriate queue families!");
        }
    }

    void Device::CreateLogicalDevice()
    {
        std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
        std::set<uint32_t> uniqueQueueFamilies = { m_QueueFamilyIndices.graphicsFamilyIndex, m_QueueFamilyIndices.presentFamilyIndex };

        float queuePriority = 1.0f;
        for (uint32_t queueFamily : uniqueQueueFamilies) {
            VkDeviceQueueCreateInfo queueCreateInfo{};
            queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queueCreateInfo.queueFamilyIndex = queueFamily;
            queueCreateInfo.queueCount = 1;
            queueCreateInfo.pQueuePriorities = &queuePriority;
            queueCreateInfos.push_back(queueCreateInfo);
        }

        /* Enable device features */
        /* using pNext chain      */
        VkPhysicalDeviceDescriptorIndexingFeatures descriptorIndexingFeatures{};
        descriptorIndexingFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES;
        descriptorIndexingFeatures.pNext = nullptr;
        descriptorIndexingFeatures.descriptorBindingPartiallyBound = VK_TRUE;
        descriptorIndexingFeatures.runtimeDescriptorArray = VK_TRUE;
        descriptorIndexingFeatures.descriptorBindingSampledImageUpdateAfterBind = VK_TRUE;
        descriptorIndexingFeatures.descriptorBindingVariableDescriptorCount = VK_TRUE;

        VkPhysicalDeviceSynchronization2Features syncronization2Features{};
        syncronization2Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES;
        syncronization2Features.pNext = &descriptorIndexingFeatures;
        syncronization2Features.synchronization2 = VK_TRUE;

        VkPhysicalDeviceHostQueryResetFeatures resetFeatures = {};
        resetFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_HOST_QUERY_RESET_FEATURES;
        resetFeatures.pNext = &syncronization2Features;
        resetFeatures.hostQueryReset = VK_TRUE;

        VkPhysicalDeviceFeatures deviceFeatures{};
        deviceFeatures.imageCubeArray = VK_TRUE;
        deviceFeatures.depthClamp = VK_TRUE;
        deviceFeatures.depthBiasClamp = VK_TRUE;
        deviceFeatures.depthBounds = VK_TRUE;
        deviceFeatures.fillModeNonSolid = VK_TRUE;

        VkPhysicalDeviceFeatures2 deviceFeatures2{};
        deviceFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
        deviceFeatures2.pNext = &resetFeatures;
        deviceFeatures2.features = deviceFeatures;
        // Fetch all features from physical device
        //vkGetPhysicalDeviceFeatures2(_physicalDevice, &deviceFeatures2);

        uint32_t supportedExtensionCount = 0;
        vkEnumerateDeviceExtensionProperties(m_PhysicalDevice, nullptr, &supportedExtensionCount, nullptr);
        std::vector<VkExtensionProperties> supportedExtensions(supportedExtensionCount);
        vkEnumerateDeviceExtensionProperties(m_PhysicalDevice, nullptr, &supportedExtensionCount, supportedExtensions.data());

        std::vector<const char*> deviceExtensions =
        {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME,
            VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME,
        };

        VkDeviceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        createInfo.pNext = &deviceFeatures2;
        createInfo.pQueueCreateInfos = queueCreateInfos.data();
        createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
        createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
        createInfo.ppEnabledExtensionNames = deviceExtensions.data();

        VK_CHECK(vkCreateDevice(m_PhysicalDevice, &createInfo, nullptr, &m_LogicalDevice));
    }

    void Device::GetDeviceQueue()
    {
        vkGetDeviceQueue(m_LogicalDevice, m_QueueFamilyIndices.graphicsFamilyIndex, 0, &m_GraphicsQueue);
        vkGetDeviceQueue(m_LogicalDevice, m_QueueFamilyIndices.presentFamilyIndex, 0, &m_PresentQueue);
    }

    void Device::CreateCommandPool()
    {
        VkCommandPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        poolInfo.queueFamilyIndex = m_QueueFamilyIndices.graphicsFamilyIndex;

        VK_CHECK(vkCreateCommandPool(m_LogicalDevice, &poolInfo, nullptr, &m_CommandPool));
    }

    void Device::CreateSurface(void* windowHandle)
    {

        VK_CHECK(glfwCreateWindowSurface(m_Instance, (GLFWwindow*)windowHandle, nullptr, &m_Surface));

        VkSurfaceCapabilitiesKHR capabilities;
        VK_CHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_PhysicalDevice, m_Surface, &capabilities));
        m_ImageCount = std::max(capabilities.minImageCount, std::min(capabilities.maxImageCount, m_ImageCount));
    }

    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageType,
        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
        void* pUserData) {

        switch (messageSeverity)
        {
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
            ARC_LOG_ERROR(pCallbackData->pMessage);

            break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
            ARC_LOG_WARNING(pCallbackData->pMessage);

            break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:

            break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:

            break;
        }

        return VK_FALSE;
    }
}