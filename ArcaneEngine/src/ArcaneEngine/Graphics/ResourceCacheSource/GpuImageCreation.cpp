#include "ArcaneEngine/Graphics/ResourceCache.h"
#include "ArcaneEngine/Core/Log.h"
#include "ArcaneEngine/Graphics/VulkanCore/VulkanLocal.h"
#include <vulkan/vulkan_core.h>
#include <vk_mem_alloc.h>

namespace Arc
{
	void ResourceCache::CreateGpuImage(GpuImage* gpuImage, const GpuImageDesc& desc)
	{

        //image->m_MipLevels = 1;
        //if (imageDescription.MipLevelsEnabled)
        //    image->m_MipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(image->m_Width, image->m_Height)))) + 1;

        VkImageType imageType = VK_IMAGE_TYPE_2D;
        VkImageViewType imageViewType = VK_IMAGE_VIEW_TYPE_2D;
        if (desc.Extent[2] > 1)
        {
            imageType = VK_IMAGE_TYPE_3D;
            imageViewType = VK_IMAGE_VIEW_TYPE_3D;
        }

        VkImageCreateInfo imageInfo = {};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.pNext = nullptr;
        imageInfo.imageType = imageType;
        imageInfo.format = (VkFormat)desc.Format;
        imageInfo.extent.width = desc.Extent[0];
        imageInfo.extent.height = desc.Extent[1];
        imageInfo.extent.depth = desc.Extent[2];
        imageInfo.mipLevels = desc.MipLevels;
        imageInfo.arrayLayers = 1;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageInfo.usage = (VkImageUsageFlags)desc.UsageFlags;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.flags = 0; // Optional

        if (desc.MipLevels > 1)
            imageInfo.usage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;

        VmaAllocationCreateInfo allocInfo = {};
        allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
        allocInfo.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

        VkImage image;
        VmaAllocation allocation;
        VK_CHECK(vmaCreateImage((VmaAllocator)m_Allocator, &imageInfo, &allocInfo, &image, &allocation, nullptr));
        gpuImage->m_Image = image;
        gpuImage->m_Allocation = allocation;

        VkImageViewCreateInfo imageViewInfo{};
        imageViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        imageViewInfo.image = image;
        imageViewInfo.viewType = imageViewType;
        imageViewInfo.format = (VkFormat)desc.Format;
        imageViewInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewInfo.subresourceRange.aspectMask = (VkImageAspectFlags)desc.AspectFlags;
        imageViewInfo.subresourceRange.baseMipLevel = 0;
        imageViewInfo.subresourceRange.levelCount = desc.MipLevels;
        imageViewInfo.subresourceRange.baseArrayLayer = 0;
        imageViewInfo.subresourceRange.layerCount = 1;
        
        VkImageView imageView;
        VK_CHECK(vkCreateImageView((VkDevice)m_LogicalDevice, &imageViewInfo, nullptr, &imageView));
        gpuImage->m_ImageView = imageView;
        gpuImage->m_Format = desc.Format;
        gpuImage->m_Extent[0] = desc.Extent[0];
        gpuImage->m_Extent[1] = desc.Extent[1];
        gpuImage->m_Extent[2] = desc.Extent[2];

        m_Resources[gpuImage] = ResourceType::GpuImage;
    }


    void ResourceCache::ReleaseResource(GpuImage* gpuImage)
    {
        if (!m_Resources.contains(gpuImage))
        {
            ARC_LOG_FATAL("Cannot find GpuImage resource to release!");
        }
        vmaDestroyImage((VmaAllocator)m_Allocator, (VkImage)gpuImage->m_Image, (VmaAllocation)gpuImage->m_Allocation);
        vkDestroyImageView((VkDevice)m_LogicalDevice, (VkImageView)gpuImage->m_ImageView, nullptr);
        m_Resources.erase(gpuImage);
    }
}