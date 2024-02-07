#include "PresentQueue.h"

namespace Arc
{

	PresentQueue::PresentQueue(Device* device, 
		SurfaceDesc windowDesc,
		PresentMode preferredMode)
	{
		m_Device = device->GetLogicalDevice();
		m_GraphicsQueue = device->GetGraphicsQueue();

		m_Swapchain = device->CreateSwapchain(windowDesc, preferredMode);
		m_FrameIndex = 0;

		VkSemaphoreCreateInfo semaphoreInfo = {};
		semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		VkFenceCreateInfo fenceInfo = {};
		fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

		uint32_t imageCount = device->GetImageCount();
		m_Frames.resize(imageCount);
		for (int i = 0; i < imageCount; i++)
		{
			VK_CHECK(vkCreateSemaphore(m_Device, &semaphoreInfo, nullptr, &m_Frames[i].imageAcquiredSemaphore));
			VK_CHECK(vkCreateSemaphore(m_Device, &semaphoreInfo, nullptr, &m_Frames[i].renderingFinishedSemaphore));
			VK_CHECK(vkCreateFence(m_Device, &fenceInfo, nullptr, &m_Frames[i].inFlightFence));
			m_Frames[i].commandBuffer.Create(m_Device, device->GetCommandPool());
		}
	}

	PresentQueue::~PresentQueue()
	{
		for (int i = 0; i < m_Frames.size(); i++)
		{
			vkDestroySemaphore(m_Device, m_Frames[i].imageAcquiredSemaphore, nullptr);
			vkDestroySemaphore(m_Device, m_Frames[i].renderingFinishedSemaphore, nullptr);
			vkDestroyFence(m_Device, m_Frames[i].inFlightFence, nullptr);
		}
	}

	FrameData PresentQueue::BeginFrame()
	{
		VK_CHECK(vkWaitForFences(
			m_Device,
			1,
			&m_Frames[m_FrameIndex].inFlightFence,
			VK_TRUE,
			std::numeric_limits<uint64_t>::max()));

		uint32_t swapchainImageIndex = m_Swapchain->AcquireNextImage(m_Frames[m_FrameIndex].imageAcquiredSemaphore);
	
		CommandBuffer* cmd = &m_Frames[m_FrameIndex].commandBuffer;
		
		cmd->Begin();

		FrameData frameData;
		frameData.CommandBuffer = cmd;
		frameData.FrameIndex = m_FrameIndex;
		frameData.m_SwapchainImageIndex = swapchainImageIndex;

		return frameData;
	
	}

	void PresentQueue::EndFrame()
	{
		CommandBuffer* cmd = &m_Frames[m_FrameIndex].commandBuffer;
		cmd->End();

		VkSubmitInfo submitInfo = {};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

		VkSemaphore waitSemaphores[] = { m_Frames[m_FrameIndex].imageAcquiredSemaphore };
		VkSemaphore signalSemaphores[] = { m_Frames[m_FrameIndex].renderingFinishedSemaphore };
		VkCommandBuffer commandBuffers[] = { cmd->GetHandle() };

		VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = waitSemaphores;
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = signalSemaphores;
		submitInfo.pWaitDstStageMask = waitStages;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = commandBuffers;

		VK_CHECK(vkResetFences(m_Device, 1, &m_Frames[m_FrameIndex].inFlightFence));
		VK_CHECK(vkQueueSubmit(m_GraphicsQueue, 1, &submitInfo, m_Frames[m_FrameIndex].inFlightFence));

		m_Swapchain->PresentImage(m_Frames[m_FrameIndex].renderingFinishedSemaphore);
		m_FrameIndex = (m_FrameIndex + 1) % m_Frames.size();
	}

}