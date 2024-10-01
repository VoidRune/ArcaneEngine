#include "PresentQueue.h"
#include "ArcaneEngine/Core/Log.h"
#include "ArcaneEngine/Graphics/VulkanCore/VulkanHandleCreation.h"
#include "ArcaneEngine/Graphics/VulkanCore/VulkanLocal.h"
#include <vulkan/vulkan_core.h>

namespace Arc
{
	PresentQueue::PresentQueue(Device* device, PresentMode presentMode)
	{
		if (!device)
		{
			ARC_LOG_FATAL("Failed to create ResourceCache object: Device pointer is not valid!");
		}

		m_LogicalDevice = device->GetLogicalDevice();
		m_PresentQueue = device->GetPresentQueue();
		SwapchainCreateInfo swapchainCreateInfo =
		{
			.instance = device->GetInstance(),
			.physicalDevice = device->GetPhysicalDevice(),
			.logicalDevice = m_LogicalDevice,
			.surface = device->GetSurface(),
			.queueFamilyIndices = device->GetQueueFamilyIndices(),
			.imageCount = device->GetFramesInFlightCount() ,
		};

		SwapchainOutput swapchainOutput = CreateSwapchainHandle(swapchainCreateInfo);
		m_Swapchain = swapchainOutput.swapchain;
		m_ImageCount = swapchainOutput.imageCount;
		m_SurfaceFormat = swapchainOutput.surfaceFormat;
		m_Extent[0] = swapchainOutput.extent[0];
		m_Extent[1] = swapchainOutput.extent[1];
		m_Extent[2] = swapchainOutput.extent[2];

		SwapchainImagesRetreiveInfo swapchainImageRetreiveInfo =
		{
			.logicalDevice = m_LogicalDevice,
			.swapchain = m_Swapchain,
		};

		m_SwapchainImages = RetreiveSwapchainImages(swapchainImageRetreiveInfo);

		m_SwapchainImageViews.resize(m_SwapchainImages.size());
		for (size_t i = 0; i < m_SwapchainImageViews.size(); i++) {
			VkImageViewCreateInfo createInfo{};
			createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			createInfo.image = (VkImage)m_SwapchainImages[i];
			createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
			createInfo.format = (VkFormat)swapchainOutput.surfaceFormat;
			createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			createInfo.subresourceRange.baseMipLevel = 0;
			createInfo.subresourceRange.levelCount = 1;
			createInfo.subresourceRange.baseArrayLayer = 0;
			createInfo.subresourceRange.layerCount = 1;

			VkImageView imageView;
			VK_CHECK(vkCreateImageView((VkDevice)m_LogicalDevice, &createInfo, nullptr, &imageView));
			m_SwapchainImageViews[i] = imageView;
		}

		VkSemaphoreCreateInfo semaphoreInfo = {};
		semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		VkFenceCreateInfo fenceInfo = {};
		fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

		m_FrameResources.reserve(m_ImageCount);
		for (int i = 0; i < m_ImageCount; i++)
		{
			SemaphoreCreateInfo semaphoreCreateInfo = {
				.logicalDevice = m_LogicalDevice
			};
			FenceCreateInfo fenceCreateInfo = {
				.logicalDevice = m_LogicalDevice,
				.createSignaled = true,
			};

			FrameResources resources = {
				.imageAcquiredSemaphore = CreateSemaphoreHandle(semaphoreCreateInfo),
				.renderingFinishedSemaphore = CreateSemaphoreHandle(semaphoreCreateInfo),
				.inFlightFence = CreateFenceHandle(fenceCreateInfo),
				.commandBuffer = CommandBuffer(m_LogicalDevice, device->GetCommanPool())
			};
			m_FrameResources.push_back(resources);
		}

		m_OutOfDate = false;
		m_FrameIndex = 0;
	}

	PresentQueue::~PresentQueue()
	{
		for (int i = 0; i < m_FrameResources.size(); i++)
		{
			vkDestroyFence((VkDevice)m_LogicalDevice, (VkFence)m_FrameResources[i].inFlightFence, nullptr);
			vkDestroySemaphore((VkDevice)m_LogicalDevice, (VkSemaphore)m_FrameResources[i].imageAcquiredSemaphore, nullptr);
			vkDestroySemaphore((VkDevice)m_LogicalDevice, (VkSemaphore)m_FrameResources[i].renderingFinishedSemaphore, nullptr);
		}
		m_FrameResources.clear();

		for (size_t i = 0; i < m_SwapchainImageViews.size(); i++)
		{
			vkDestroyImageView((VkDevice)m_LogicalDevice, (VkImageView)m_SwapchainImageViews[i], nullptr);
		}

		vkDestroySwapchainKHR((VkDevice)m_LogicalDevice,(VkSwapchainKHR)m_Swapchain, nullptr);
	}

	FrameData PresentQueue::BeginFrame()
	{
		VkFence fence = (VkFence)m_FrameResources[m_FrameIndex].inFlightFence;
		VK_CHECK(vkWaitForFences(
			(VkDevice)m_LogicalDevice,
			1,
			&fence,
			VK_TRUE,
			std::numeric_limits<uint64_t>::max()));

		AcquireNextImage();

		CommandBuffer* cmd = &m_FrameResources[m_FrameIndex].commandBuffer;
		
		cmd->Begin();
		
		FrameData frameData;
		frameData.CommandBuffer = cmd;
		frameData.FrameIndex = m_FrameIndex;
		frameData.PresentImage = m_SwapchainImages[m_PresentImageIndex];
		frameData.PresentImageView = m_SwapchainImageViews[m_PresentImageIndex];

		return frameData;
	}

	void PresentQueue::EndFrame()
	{
		if (m_OutOfDate)
			return;

		CommandBuffer* cmd = &m_FrameResources[m_FrameIndex].commandBuffer;
		cmd->End();

		VkSubmitInfo2 submitInfo2 = {};
		submitInfo2.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;

		VkSemaphoreSubmitInfo waitSemaphoreInfo = {};
		waitSemaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
		waitSemaphoreInfo.pNext = nullptr;
		waitSemaphoreInfo.semaphore = (VkSemaphore)m_FrameResources[m_FrameIndex].imageAcquiredSemaphore;
		waitSemaphoreInfo.stageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT_KHR;
		waitSemaphoreInfo.deviceIndex = 0;
		waitSemaphoreInfo.value = 1;

		submitInfo2.waitSemaphoreInfoCount = 1;
		submitInfo2.pWaitSemaphoreInfos = &waitSemaphoreInfo;

		VkSemaphoreSubmitInfo signalSemaphoreInfo = {};
		signalSemaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
		signalSemaphoreInfo.pNext = nullptr;
		signalSemaphoreInfo.semaphore = (VkSemaphore)m_FrameResources[m_FrameIndex].renderingFinishedSemaphore;
		signalSemaphoreInfo.stageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT; //VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT
		signalSemaphoreInfo.deviceIndex = 0;
		signalSemaphoreInfo.value = 1;

		submitInfo2.signalSemaphoreInfoCount = 1;
		submitInfo2.pSignalSemaphoreInfos = &signalSemaphoreInfo;

		VkCommandBufferSubmitInfo commandBufferSubmitInfo{};
		commandBufferSubmitInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
		commandBufferSubmitInfo.pNext = nullptr;
		commandBufferSubmitInfo.commandBuffer = (VkCommandBuffer)cmd->GetHandle();
		commandBufferSubmitInfo.deviceMask = 0;

		submitInfo2.commandBufferInfoCount = 1;
		submitInfo2.pCommandBufferInfos = &commandBufferSubmitInfo;

		VkFence inFlightFence = (VkFence)m_FrameResources[m_FrameIndex].inFlightFence;
		VK_CHECK(vkResetFences((VkDevice)m_LogicalDevice, 1, &inFlightFence));
		VK_CHECK(vkQueueSubmit2((VkQueue)m_PresentQueue, 1, &submitInfo2, inFlightFence));

		PresentImage();
		m_FrameIndex = (m_FrameIndex + 1) % m_ImageCount;
	}

	void PresentQueue::AcquireNextImage()
	{
		VkResult err = vkAcquireNextImageKHR(
			(VkDevice)m_LogicalDevice,
			(VkSwapchainKHR)m_Swapchain,
			std::numeric_limits<uint64_t>::max(),
			(VkSemaphore)m_FrameResources[m_FrameIndex].imageAcquiredSemaphore,
			VK_NULL_HANDLE,
			&m_PresentImageIndex);

		if (err == VK_ERROR_OUT_OF_DATE_KHR)
		{
			m_OutOfDate = true;
		}
	}

	void PresentQueue::PresentImage()
	{
		VkPresentInfoKHR presentInfo = {};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

		VkSemaphore renderingFinishedSemaphore = (VkSemaphore)m_FrameResources[m_FrameIndex].renderingFinishedSemaphore;
		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = &renderingFinishedSemaphore;

		presentInfo.swapchainCount = 1;
		VkSwapchainKHR swapchain = (VkSwapchainKHR)m_Swapchain;
		presentInfo.pSwapchains = &swapchain;
		presentInfo.pImageIndices = &m_PresentImageIndex;

		VkResult err = vkQueuePresentKHR((VkQueue)m_PresentQueue, &presentInfo);

		if (err == VK_ERROR_OUT_OF_DATE_KHR || err == VK_SUBOPTIMAL_KHR)
		{
			m_OutOfDate = true;
		}
		else
		{
			VK_CHECK(err);
		}
	}

}