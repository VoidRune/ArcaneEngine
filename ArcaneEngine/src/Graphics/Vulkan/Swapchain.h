#pragma once
#include "VkLocal.h"

namespace Arc
{
	struct QueueFamilyIndices;
	struct SurfaceDesc;
	class Swapchain
	{
	public:
		Swapchain(
			VkInstance instance, VkDevice device, VkPhysicalDevice physicalDevice,
			SurfaceDesc windowDesc,
			QueueFamilyIndices gueueFamilyIndices,
			VkQueue presentQueue,
			uint32_t imagesCount,
			VkPresentModeKHR preferredMode);
		~Swapchain();

		uint32_t AcquireNextImage(VkSemaphore semaphore);
		void PresentImage(VkSemaphore semaphore);

		VkSwapchainKHR GetHandle() { return m_Swapchain; };
		std::vector<VkImageView> GetImageViews() { return m_ImageViews; }

		VkSurfaceFormatKHR GetSurfaceFormat() { return m_SurfaceFormat; };
		VkPresentModeKHR GetPresentMode() { return m_PresentMode; };
		VkExtent2D GetExtent() { return m_Extent; };

		bool OutOfDate() { return m_OutOfDate; }
	private:

		void CreateSurface(VkInstance instance, SurfaceDesc windowDesc);
		void CreateSwapchain(VkDevice device, VkPhysicalDevice physicalDevice, QueueFamilyIndices gueueFamilyIndices, uint32_t imagesCount, VkPresentModeKHR preferredMode);
		void CreateImageViews();

		VkSurfaceFormatKHR FindSurfaceFormat(VkPhysicalDevice physicalDevice, VkFormat preferredFormat);
		VkPresentModeKHR FindPresentMode(VkPhysicalDevice physicalDevice, VkPresentModeKHR preferredMode);
		VkExtent2D FindSurfaceExtent(VkPhysicalDevice physicalDevice, VkExtent2D extent);


		VkSwapchainKHR m_Swapchain;
		VkSurfaceKHR m_Surface;
		VkInstance m_Instance;
		VkDevice m_LogicalDevice;
		VkQueue m_PresentQueue;
		bool m_OutOfDate;

		VkSurfaceFormatKHR m_SurfaceFormat;
		VkPresentModeKHR m_PresentMode;
		VkExtent2D m_Extent;

		uint32_t m_ImageCount;
		uint32_t m_ImageIndex;

		std::vector<VkImage> m_Images;
		std::vector<VkImageView> m_ImageViews;
	};
}