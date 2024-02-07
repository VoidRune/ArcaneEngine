#pragma once
#include "Vulkan/VkLocal.h"
#include "Device.h"
#include "Vulkan/CommandBuffer.h"

namespace Arc
{
	class Device;
	struct SurfaceDesc;
	class Swapchain;
	class PresentQueue
	{
	public:
		PresentQueue(Device* core, SurfaceDesc windowDesc,
			PresentMode preferredMode);
		~PresentQueue();

		FrameData BeginFrame();
		void EndFrame();

		Swapchain* GetSwapchain() { return m_Swapchain.get(); }
		uint32_t GetCurrentFrameIndex() { return m_FrameIndex; }

	private:

		struct FrameResources
		{
			VkSemaphore imageAcquiredSemaphore;
			VkSemaphore renderingFinishedSemaphore;
			VkFence inFlightFence;
			CommandBuffer commandBuffer;
		};
		std::vector<FrameResources> m_Frames;
		uint32_t m_FrameIndex;

		VkDevice m_Device;
		VkQueue m_GraphicsQueue;
		std::unique_ptr<Swapchain> m_Swapchain;
	};
}