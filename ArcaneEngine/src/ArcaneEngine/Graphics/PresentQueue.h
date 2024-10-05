#pragma once
#include "ArcaneEngine/Graphics/CommandBuffer.h"
#include "ArcaneEngine/Graphics/Common.h"

namespace Arc
{
	class FrameData
	{
	public:
		CommandBuffer* CommandBuffer;
		uint32_t FrameIndex;
		ImageHandle PresentImage;
		ImageViewHandle PresentImageView;
	};

	class Device;
	class PresentQueue
	{
	public:
		PresentQueue(Device* device, PresentMode presentMode);
		~PresentQueue();

		FrameData BeginFrame();
		void EndFrame();
		bool OutOfDate() { return m_OutOfDate; };

		Format GetSurfaceFormat() { return m_SurfaceFormat; }
		ImageHandle GetImage(uint32_t index) { return m_SwapchainImages[index]; };
		uint32_t* GetExtent() { return m_Extent; };
		ImageViewHandle GetImageView(uint32_t index) { return m_SwapchainImageViews[index]; };

	private:

		void AcquireNextImage();
		void PresentImage();

		struct FrameResources
		{
			SemaphoreHandle imageAcquiredSemaphore;
			SemaphoreHandle renderingFinishedSemaphore;
			FenceHandle inFlightFence;
			CommandBuffer commandBuffer;
		};
		std::vector<FrameResources> m_FrameResources;

		DeviceHandle m_LogicalDevice;
		SwapchainHandle m_Swapchain;
		QueueHandle m_PresentQueue;
		uint32_t m_ImageCount;
		Format m_SurfaceFormat;
		uint32_t m_Extent[3];
		std::vector<ImageHandle> m_SwapchainImages;
		std::vector<ImageViewHandle> m_SwapchainImageViews;
		bool m_OutOfDate;
		uint32_t m_FrameIndex;
		uint32_t m_PresentImageIndex;
	};
}