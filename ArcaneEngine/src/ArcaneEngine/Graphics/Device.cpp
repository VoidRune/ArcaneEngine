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
        m_TimestampQuery = std::make_unique<TimestampQuery>(m_LogicalDevice, m_PhysicalDevice);
    }

	Device::~Device()
	{
        m_TimestampQuery.reset();
        m_RenderGraph.reset();
        m_ResourceCache.reset();

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
            writeInfo.dstArrayElement = bw.ArrayElement;
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

    std::vector<uint8_t> Device::GetImageData(GpuImage* image)
    {
        bool screenshotSaved = false;
        bool supportsBlit = true;

        // Check blit support for source and destination
        VkFormatProperties formatProps;

        // Check if the device supports blitting from optimal images (the swapchain images are in optimal format)
        vkGetPhysicalDeviceFormatProperties((VkPhysicalDevice)m_PhysicalDevice, (VkFormat)image->GetFormat(), &formatProps);
        if (!(formatProps.optimalTilingFeatures & VK_FORMAT_FEATURE_BLIT_SRC_BIT)) {
            supportsBlit = false;
        }
        // Check if the device supports blitting to linear images
        vkGetPhysicalDeviceFormatProperties((VkPhysicalDevice)m_PhysicalDevice, VK_FORMAT_R8G8B8A8_UNORM, &formatProps);
        if (!(formatProps.linearTilingFeatures & VK_FORMAT_FEATURE_BLIT_DST_BIT)) {
            supportsBlit = false;
        }

        VkImage srcImage = (VkImage)image->GetHandle();
        uint32_t width = image->GetExtent()[0];
        uint32_t height = image->GetExtent()[1];

        // Create the linear tiled destination image to copy to and to read the memory from
        VkImageCreateInfo imageCreateCI = {};
        imageCreateCI.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageCreateCI.imageType = VK_IMAGE_TYPE_2D;
        // Note that vkCmdBlitImage (if supported) will also do format conversions if the swapchain color format would differ
        imageCreateCI.format = VK_FORMAT_R8G8B8A8_UNORM;
        imageCreateCI.extent.width = width;
        imageCreateCI.extent.height = height;
        imageCreateCI.extent.depth = 1;
        imageCreateCI.arrayLayers = 1;
        imageCreateCI.mipLevels = 1;
        imageCreateCI.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageCreateCI.samples = VK_SAMPLE_COUNT_1_BIT;
        imageCreateCI.tiling = VK_IMAGE_TILING_LINEAR;
        imageCreateCI.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        // Create the image
        VkImage dstImage;
        vkCreateImage((VkDevice)m_LogicalDevice, &imageCreateCI, nullptr, &dstImage);
        // Create memory to back up the image
        VkMemoryRequirements memRequirements;
        VkMemoryAllocateInfo memAllocInfo{};
        memAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        VkDeviceMemory dstImageMemory;
        vkGetImageMemoryRequirements((VkDevice)m_LogicalDevice, dstImage, &memRequirements);
        memAllocInfo.allocationSize = memRequirements.size;
        // Memory must be host visible to copy from
        
        VkPhysicalDeviceMemoryProperties memProps;
        vkGetPhysicalDeviceMemoryProperties((VkPhysicalDevice)m_PhysicalDevice, &memProps);
        uint32_t typeBits = memRequirements.memoryTypeBits;
        uint32_t properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
        for (uint32_t i = 0; i < memProps.memoryTypeCount; i++)
        {
            if ((typeBits & 1) == 1)
            {
                if ((memProps.memoryTypes[i].propertyFlags & properties) == properties)
                {
                    memAllocInfo.memoryTypeIndex = i;
                    break;
                }
            }
            typeBits >>= 1;
        }

        vkAllocateMemory((VkDevice)m_LogicalDevice, &memAllocInfo, nullptr, &dstImageMemory);
        vkBindImageMemory((VkDevice)m_LogicalDevice, dstImage, dstImageMemory, 0);

        // Do the actual blit from the swapchain image to our host visible destination image
        //VkCommandBuffer copyCmd = vulkanDevice->createCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);

        ImmediateSubmit([=](CommandBufferHandle cmd) {
            // Transition destination image to transfer destination layout
            VkImageMemoryBarrier imageMemoryBarrier = {};
            imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            imageMemoryBarrier.srcAccessMask = 0;
            imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            imageMemoryBarrier.image = dstImage;
            imageMemoryBarrier.subresourceRange = VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };

            vkCmdPipelineBarrier(
                (VkCommandBuffer)cmd,
                VK_PIPELINE_STAGE_TRANSFER_BIT,
                VK_PIPELINE_STAGE_TRANSFER_BIT,
                0,
                0, nullptr,
                0, nullptr,
                1, &imageMemoryBarrier);

            // Transition swapchain image from present to transfer source layout
            imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            imageMemoryBarrier.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
            imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
            imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
            imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            imageMemoryBarrier.image = srcImage;
            imageMemoryBarrier.subresourceRange = VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };

            vkCmdPipelineBarrier(
                (VkCommandBuffer)cmd,
                VK_PIPELINE_STAGE_TRANSFER_BIT,
                VK_PIPELINE_STAGE_TRANSFER_BIT,
                0,
                0, nullptr,
                0, nullptr,
                1, &imageMemoryBarrier);

            // If source and destination support blit we'll blit as this also does automatic format conversion (e.g. from BGR to RGB)
            if (supportsBlit)
            {
                // Define the region to blit (we will blit the whole swapchain image)
                VkOffset3D blitSize;
                blitSize.x = width;
                blitSize.y = height;
                blitSize.z = 1;
                VkImageBlit imageBlitRegion{};
                imageBlitRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                imageBlitRegion.srcSubresource.layerCount = 1;
                imageBlitRegion.srcOffsets[1] = blitSize;
                imageBlitRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                imageBlitRegion.dstSubresource.layerCount = 1;
                imageBlitRegion.dstOffsets[1] = blitSize;

                // Issue the blit command
                vkCmdBlitImage(
                    (VkCommandBuffer)cmd,
                    srcImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                    dstImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                    1,
                    &imageBlitRegion,
                    VK_FILTER_NEAREST);
            }
            else
            {
                // Otherwise use image copy (requires us to manually flip components)
                VkImageCopy imageCopyRegion{};
                imageCopyRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                imageCopyRegion.srcSubresource.layerCount = 1;
                imageCopyRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                imageCopyRegion.dstSubresource.layerCount = 1;
                imageCopyRegion.extent.width = width;
                imageCopyRegion.extent.height = height;
                imageCopyRegion.extent.depth = 1;

                // Issue the copy command
                vkCmdCopyImage(
                    (VkCommandBuffer)cmd,
                    srcImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                    dstImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                    1,
                    &imageCopyRegion);
            }

            // Transition destination image to general layout, which is the required layout for mapping the image memory later on

            imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            imageMemoryBarrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
            imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
            imageMemoryBarrier.image = dstImage;
            imageMemoryBarrier.subresourceRange = VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };

            vkCmdPipelineBarrier(
                (VkCommandBuffer)cmd,
                VK_PIPELINE_STAGE_TRANSFER_BIT,
                VK_PIPELINE_STAGE_TRANSFER_BIT,
                0,
                0, nullptr,
                0, nullptr,
                1, &imageMemoryBarrier);

            // Transition back the swap chain image after the blit is done

            imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
            imageMemoryBarrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
            imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
            imageMemoryBarrier.image = dstImage;
            imageMemoryBarrier.subresourceRange = VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };

            vkCmdPipelineBarrier(
                (VkCommandBuffer)cmd,
                VK_PIPELINE_STAGE_TRANSFER_BIT,
                VK_PIPELINE_STAGE_TRANSFER_BIT,
                0,
                0, nullptr,
                0, nullptr,
                1, &imageMemoryBarrier);
            });

        // Get layout of the image (including row pitch)
        VkImageSubresource subResource{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 0 };
        VkSubresourceLayout subResourceLayout;
        vkGetImageSubresourceLayout((VkDevice)m_LogicalDevice, dstImage, &subResource, &subResourceLayout);

        // Map image memory so we can start copying from it
        uint8_t* dataPtr;
        vkMapMemory((VkDevice)m_LogicalDevice, dstImageMemory, 0, VK_WHOLE_SIZE, 0, (void**)&dataPtr);
        dataPtr += subResourceLayout.offset;
        std::vector<uint8_t> data = std::vector<uint8_t>(dataPtr, dataPtr + width * height * 4);
        vkUnmapMemory((VkDevice)m_LogicalDevice, dstImageMemory);
        vkFreeMemory((VkDevice)m_LogicalDevice, dstImageMemory, nullptr);
        vkDestroyImage((VkDevice)m_LogicalDevice, dstImage, nullptr);

        return std::move(data);
        //bool colorSwizzle = false;
        //if (!supportsBlit)
        //{
        //    std::vector<VkFormat> formatsBGR = { VK_FORMAT_B8G8R8A8_SRGB, VK_FORMAT_B8G8R8A8_UNORM, VK_FORMAT_B8G8R8A8_SNORM };
        //    colorSwizzle = (std::find(formatsBGR.begin(), formatsBGR.end(), _swapchain->GetSurfaceFormat().format) != formatsBGR.end());
        //}
    }

    void Device::ClearColorImage(GpuImage* image, float clearColor[4], ImageLayout layout)
    {
        ImmediateSubmit([&](BufferHandle cmd) { 
            VkClearColorValue clear{ clearColor[0], clearColor[1], clearColor[2], clearColor[3] };
            VkImageSubresourceRange range;
            range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            range.baseMipLevel = 0;
            range.levelCount = image->GetMipLevels();
            range.baseArrayLayer = 0;
            range.layerCount = 1;
            vkCmdClearColorImage((VkCommandBuffer)cmd, (VkImage)image->GetHandle(), (VkImageLayout)layout, &clear, 1, &range);
        });

    }

    void Device::SetImageData(GpuImage* image, const void* data, uint32_t size, ImageLayout newLayout)
    {
        GpuBuffer stagingBuffer;
        m_ResourceCache->CreateGpuBuffer(&stagingBuffer, GpuBufferDesc{
            .Size = size,
            .UsageFlags = Arc::BufferUsage::TransferSrc,
            .MemoryProperty = Arc::MemoryProperty::HostVisible,
        });

        void* dataPtr = m_ResourceCache->MapMemory(&stagingBuffer);
        memcpy(dataPtr, data, size);
        m_ResourceCache->UnmapMemory(&stagingBuffer);

        ImmediateSubmit([&](BufferHandle cmd) {
            VkImageMemoryBarrier barrier{};
            barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.image = (VkImage)image->GetHandle();
            barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            barrier.subresourceRange.baseMipLevel = 0;
            barrier.subresourceRange.levelCount = image->GetMipLevels();
            barrier.subresourceRange.baseArrayLayer = 0;
            barrier.subresourceRange.layerCount = 1;
            barrier.srcAccessMask = 0;
            barrier.dstAccessMask = 0;

            vkCmdPipelineBarrier((VkCommandBuffer)cmd, VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, NULL, 0, NULL, 1, &barrier);

            VkBufferImageCopy region = {};
            region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            region.imageSubresource.layerCount = 1;
            region.imageExtent.width = image->GetExtent()[0];
            region.imageExtent.height = image->GetExtent()[1];
            region.imageExtent.depth = image->GetExtent()[2];
            vkCmdCopyBufferToImage((VkCommandBuffer)cmd, (VkBuffer)stagingBuffer.GetHandle(), (VkImage)image->GetHandle(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

            VkImageMemoryBarrier use_barrier = {};
            use_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            use_barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            use_barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
            use_barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            use_barrier.newLayout = static_cast<VkImageLayout>(newLayout);
            if (image->GetMipLevels() > 1)
                use_barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            use_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            use_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            use_barrier.image = (VkImage)image->GetHandle();
            use_barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            use_barrier.subresourceRange.levelCount = 1;
            use_barrier.subresourceRange.layerCount = 1;
            vkCmdPipelineBarrier((VkCommandBuffer)cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, NULL, 0, NULL, 1, &use_barrier);
        });

        m_ResourceCache->ReleaseResource(&stagingBuffer);
    }

    void Device::CreateInstance(const std::vector<const char*>& instanceExtensions)
    {
        InstanceCreateInfo instanceCreateInfo = {
            .applicationName = "Application",
            .engineName = "Arcane Renderer",
            .apiVersion = ApiVersion(1, 4),
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