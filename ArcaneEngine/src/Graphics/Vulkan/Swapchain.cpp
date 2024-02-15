#include "Swapchain.h"
#include <algorithm>

/* Needed for surface creation */
#include <vulkan/vulkan_win32.h>

namespace Arc
{

    Swapchain::Swapchain(VkInstance instance, VkDevice device,
        VkPhysicalDevice physicalDevice, SurfaceDesc windowDesc,
        QueueFamilyIndices gueueFamilyIndices, VkQueue presentQueue,
        uint32_t imagesCount, VkPresentModeKHR preferredMode)
    {
        m_Swapchain = VK_NULL_HANDLE;
        m_Instance = instance;
        m_LogicalDevice = device;
        m_PresentQueue = presentQueue;
        CreateSurface(instance, windowDesc);
        CreateSwapchain(device, physicalDevice, gueueFamilyIndices, imagesCount, preferredMode);
        CreateImageViews();
        m_OutOfDate = false;
    }

    void Swapchain::CreateSurface(VkInstance instance, SurfaceDesc windowDesc)
    {
        VkWin32SurfaceCreateInfoKHR createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
        createInfo.hwnd = (HWND)windowDesc.hWnd;
        createInfo.hinstance = (HINSTANCE)windowDesc.hInstance;
        VK_CHECK(vkCreateWin32SurfaceKHR(instance, &createInfo, nullptr, &m_Surface));
    }

    void Swapchain::CreateSwapchain(VkDevice device, VkPhysicalDevice physicalDevice, QueueFamilyIndices gueueFamilyIndices, uint32_t imagesCount, VkPresentModeKHR preferredMode)
    {
        VkSurfaceCapabilitiesKHR capabilities;
        VK_CHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, m_Surface, &capabilities));

        m_SurfaceFormat = FindSurfaceFormat(physicalDevice, VK_FORMAT_B8G8R8A8_UNORM);
        m_PresentMode = FindPresentMode(physicalDevice, preferredMode);
        m_Extent = FindSurfaceExtent(physicalDevice, VkExtent2D{128, 128});

        uint32_t imageCount = std::max(capabilities.minImageCount, imagesCount);
        if (capabilities.maxImageCount > 0 && imageCount > capabilities.maxImageCount)
            imageCount = capabilities.maxImageCount;

        m_ImageCount = imageCount;
        m_ImageIndex = 0;

        VkSwapchainCreateInfoKHR createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        createInfo.surface = m_Surface;
        createInfo.minImageCount = m_ImageCount;
        createInfo.imageFormat = m_SurfaceFormat.format;
        createInfo.imageColorSpace = m_SurfaceFormat.colorSpace;
        createInfo.imageExtent = m_Extent;
        createInfo.imageArrayLayers = 1;
        createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

        uint32_t indices[] = { gueueFamilyIndices.graphicsFamilyIndex, gueueFamilyIndices.presentFamilyIndex };
        if (indices[0] != indices[1])
        {
            createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
            createInfo.queueFamilyIndexCount = 2;
            createInfo.pQueueFamilyIndices = indices;
        }
        else
        {
            createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
            createInfo.queueFamilyIndexCount = 0; // Optional
            createInfo.pQueueFamilyIndices = nullptr; // Optional
        }

        createInfo.preTransform = capabilities.currentTransform;
        createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        createInfo.presentMode = m_PresentMode;
        createInfo.clipped = VK_TRUE;
        createInfo.oldSwapchain = nullptr;
        //VkSwapchainKHR oldSwapchain = _swapchain;
        //createInfo.oldSwapchain = oldSwapchain;

        VK_CHECK(vkCreateSwapchainKHR(m_LogicalDevice, &createInfo, nullptr, &m_Swapchain));

        //if (oldSwapchain != VK_NULL_HANDLE)
        //    vkDestroySwapchainKHR(Core::Get()->GetLogicalDevice(), oldSwapchain, nullptr);
    }

    void Swapchain::CreateImageViews()
    {
        VK_CHECK(vkGetSwapchainImagesKHR(m_LogicalDevice, m_Swapchain, &m_ImageCount, nullptr));
        m_Images.resize(m_ImageCount);
        VK_CHECK(vkGetSwapchainImagesKHR(m_LogicalDevice, m_Swapchain, &m_ImageCount, m_Images.data()));

        m_ImageViews.resize(m_Images.size());

        for (size_t i = 0; i < m_Images.size(); i++) {
            VkImageViewCreateInfo createInfo{};
            createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            createInfo.image = m_Images[i];
            createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            createInfo.format = m_SurfaceFormat.format;
            createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            createInfo.subresourceRange.baseMipLevel = 0;
            createInfo.subresourceRange.levelCount = 1;
            createInfo.subresourceRange.baseArrayLayer = 0;
            createInfo.subresourceRange.layerCount = 1;

            VK_CHECK(vkCreateImageView(m_LogicalDevice, &createInfo, nullptr, &m_ImageViews[i]));
        }
    }

    Swapchain::~Swapchain()
    {
        for (size_t i = 0; i < m_ImageViews.size(); i++)
        {
            vkDestroyImageView(m_LogicalDevice, m_ImageViews[i], nullptr);
        }
        m_ImageViews.clear();

        vkDestroySwapchainKHR(m_LogicalDevice, m_Swapchain, nullptr);
        vkDestroySurfaceKHR(m_Instance, m_Surface, nullptr);
    }

    uint32_t Swapchain::AcquireNextImage(VkSemaphore semaphore)
    {
        VkResult err = vkAcquireNextImageKHR(
            m_LogicalDevice,
            m_Swapchain,
            std::numeric_limits<uint64_t>::max(),
            semaphore,
            VK_NULL_HANDLE,
            &m_ImageIndex);

        if (err == VK_ERROR_OUT_OF_DATE_KHR)
        {
            m_OutOfDate = true;
        }

        return m_ImageIndex;
    }

    void Swapchain::PresentImage(VkSemaphore semaphore)
    {
        VkPresentInfoKHR presentInfo = {};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = &semaphore;

        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = &m_Swapchain;
        presentInfo.pImageIndices = &m_ImageIndex;

        VkResult err = vkQueuePresentKHR(m_PresentQueue, &presentInfo);

        if (err == VK_ERROR_OUT_OF_DATE_KHR || err == VK_SUBOPTIMAL_KHR)
        {
            m_OutOfDate = true;
        }
        else
        {
            VK_CHECK(err);
        }
    }

    VkSurfaceFormatKHR Swapchain::FindSurfaceFormat(VkPhysicalDevice physicalDevice, VkFormat preferredFormat)
    {
        uint32_t formatCount;
        VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, m_Surface, &formatCount, nullptr));

        std::vector<VkSurfaceFormatKHR> formats;
        if (formatCount != 0)
        {
            formats.resize(formatCount);
            VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, m_Surface, &formatCount, formats.data()));
        }

        if (formats.size() == 1 && formats[0].format == VK_FORMAT_UNDEFINED)
        {
            VkSurfaceFormatKHR format;
            format.format = VK_FORMAT_B8G8R8A8_UNORM;
            format.colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
            return format;
        }

        for (VkSurfaceFormatKHR& availableFormat : formats) {
            if (availableFormat.format == preferredFormat &&
                availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
                return availableFormat;
            }
        }

        return formats[0];
    }

    VkPresentModeKHR Swapchain::FindPresentMode(VkPhysicalDevice physicalDevice, VkPresentModeKHR preferredMode)
    {
        uint32_t presentModeCount;
        VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, m_Surface, &presentModeCount, nullptr));

        std::vector<VkPresentModeKHR> presentModes;
        if (presentModeCount != 0)
        {
            presentModes.resize(presentModeCount);
            VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, m_Surface, &presentModeCount, presentModes.data()));
        }

        for (const auto& availablePresentMode : presentModes) {
            if (availablePresentMode == preferredMode) {
                return availablePresentMode;
            }
        }

        return VK_PRESENT_MODE_FIFO_KHR;
    }

    VkExtent2D Swapchain::FindSurfaceExtent(VkPhysicalDevice physicalDevice, VkExtent2D extent)
    {
        VkExtent2D result;

        VkSurfaceCapabilitiesKHR capabilities;
        VK_CHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, m_Surface, &capabilities));

        if (capabilities.currentExtent.width != (std::numeric_limits<uint32_t>::max)())
        {
            result = capabilities.currentExtent;
        }
        else {

            VkExtent2D actualExtent = {
                static_cast<uint32_t>(extent.width),
                static_cast<uint32_t>(extent.height)
            };
            
            //actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
            //actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

            actualExtent.width = std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, actualExtent.width));
            actualExtent.height = std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, actualExtent.height));

            result = actualExtent;
        }

        return result;
    }
}