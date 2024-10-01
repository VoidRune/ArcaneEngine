#pragma once
#include "VulkanCore/VulkanHandles.h"
#include "QueueFamilyIndices.h"
#include "VulkanObjects/DescriptorSet.h"
#include "VulkanObjects/DescriptorWrite.h"
#include <vector>

namespace Arc
{
	class Device
	{
	public:
		Device(void* windowHandle, const std::vector<const char*>& instanceExtensions, uint32_t framesInFlight);
		~Device();

		void WaitIdle();
		void UpdateDescriptorSet(DescriptorSet* descriptor, const DescriptorWrite& write);
		void UpdateDescriptorSet(DescriptorSetArray* descriptorArray, const DescriptorWrite& write);

		InstanceHandle GetInstance() { return m_Instance; }
		PhysicalDeviceHandle GetPhysicalDevice() { return m_PhysicalDevice; }
		SurfaceHandle GetSurface() { return m_Surface; }
		DeviceHandle GetLogicalDevice() { return m_LogicalDevice; }
		QueueFamilyIndices GetQueueFamilyIndices() { return m_QueueFamiliyIndices; }
		QueueHandle GetGraphicsQueue() { return m_GraphicsQueue; }
		QueueHandle GetPresentQueue() { return m_PresentQueue; }
		CommandPoolHandle GetCommanPool() { return m_CommandPool; }
		uint32_t GetFramesInFlightCount() { return m_FramesInFlight; }

	private:

		void CreateInstance(const std::vector<const char*>& instanceExtensions);
		void CreateDebugUtilsMessenger();
		void SelectPhysicalDevice();
		void CreateSurface(void* windowHandle, uint32_t framesInFlight);
		void CreateLogicalDevice();

		InstanceHandle m_Instance;
		DebugUtilsMessengerHandle m_DebugUtilsMessenger;
		PhysicalDeviceHandle m_PhysicalDevice;
		SurfaceHandle m_Surface;
		DeviceHandle m_LogicalDevice;
		uint32_t m_FramesInFlight;

		QueueFamilyIndices m_QueueFamiliyIndices;
		QueueHandle m_GraphicsQueue;
		QueueHandle m_PresentQueue;
		CommandPoolHandle m_CommandPool;
	};
}