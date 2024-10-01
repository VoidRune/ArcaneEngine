#pragma once
#include "ArcaneEngine/Graphics/VulkanCore/VulkanHandles.h"
#include "ArcaneEngine/Graphics/Common.h"
#include "ArcaneEngine/Graphics/QueueFamilyIndices.h"
#include <vector>

namespace Arc
{
	struct ApiVersion
	{
		uint32_t Major = 0;
		uint32_t Minor = 0;
	};

	struct InstanceCreateInfo
	{
		const char* applicationName = "";
		const char* engineName = "";
		ApiVersion apiVersion = ApiVersion(1, 3);
		const std::vector<const char*>& instanceExtensions = {};
		bool enableValidationLayers = false;
	};
	InstanceHandle CreateInstanceHandle(InstanceCreateInfo& info);

	struct DebugUtilsMessengerCreateInfo
	{
		InstanceHandle instance = {};
		bool enableDebugUtilsMessenger = false;
	};
	DebugUtilsMessengerHandle CreateDebugUtilsMessengerHandle(DebugUtilsMessengerCreateInfo& info);


	struct PhysicalDeviceSelectInfo
	{
		InstanceHandle instance = {};
	};
	PhysicalDeviceHandle SelectPhysicalDeviceHandle(PhysicalDeviceSelectInfo& info);

	struct SurfaceCreateInfo
	{
		InstanceHandle instance = {};
		void* windowHandle = {};
	};
	SurfaceHandle CreateSurfaceHandle(SurfaceCreateInfo& info);

	struct QueueFamilySelectInfo
	{
		PhysicalDeviceHandle physicalDevice = {};
		SurfaceHandle surface = {};
	};
	QueueFamilyIndices SelectQueueFamilies(QueueFamilySelectInfo& info);

	struct DeviceCreateInfo
	{
		PhysicalDeviceHandle physicalDevice = {};
		QueueFamilyIndices queueFamilyIndices = {};
	};
	DeviceHandle CreateLogicalDeviceHandle(DeviceCreateInfo& info);

	struct SwapchainCreateInfo
	{
		InstanceHandle instance = {};
		PhysicalDeviceHandle physicalDevice = {};
		DeviceHandle logicalDevice = {};
		SurfaceHandle surface = {};
		QueueFamilyIndices queueFamilyIndices = {};
		uint32_t imageCount = {};
		PresentMode presentMode = {};
	};

	struct SwapchainOutput
	{
		SwapchainHandle swapchain = {};
		uint32_t imageCount = {};
		Format surfaceFormat = {};
		uint32_t extent[3] = {};
	};
	SwapchainOutput CreateSwapchainHandle(SwapchainCreateInfo& info);

	struct SwapchainImagesRetreiveInfo
	{
		DeviceHandle logicalDevice = {};
		SwapchainHandle swapchain = {};
	};
	std::vector<ImageHandle> RetreiveSwapchainImages(SwapchainImagesRetreiveInfo& info);

	struct CommandBufferCreateInfo
	{
		DeviceHandle logicalDevice = {};
		CommandPoolHandle commandPool = {};
	};
	CommandBufferHandle CreateCommandBufferHandle(CommandBufferCreateInfo& info);


	struct ImageViewCreateInfo
	{
		DeviceHandle logicalDevice = {};
		ImageHandle image = {};
		Format format = {};
		ImageAspect imageAspect = {};
	};
	ImageViewHandle CreateImageViewHandle(ImageViewCreateInfo& info);

	struct FenceCreateInfo
	{
		DeviceHandle logicalDevice = {};
		bool createSignaled = {};
	};
	FenceHandle CreateFenceHandle(FenceCreateInfo& info);

	struct SemaphoreCreateInfo
	{
		DeviceHandle logicalDevice = {};
	};
	SemaphoreHandle CreateSemaphoreHandle(SemaphoreCreateInfo& info);
}