#pragma once
#include "Vulkan/VkLocal.h"
#include "Vulkan/Common.h"
#include "Vulkan/Swapchain.h"
#include "Vulkan/Image.h"
#include "Vulkan/GpuBuffer.h"
#include "RenderGraph.h"
#include "GpuProfiler.h"
#include "ResourceCache.h"

namespace Arc
{
	class Swapchain;
	class RenderGraph;
	class DescriptorSetCache;
	class Image;
	class Device
	{
	public:
		Device(void* windowHandle, uint32_t imageCount);
		~Device();

		void WaitIdle();

		void ImmediateSubmit(std::function<void(VkCommandBuffer cmd)>&& func);

		struct UpdateDescriptorSetInfo
		{
			uint32_t binding;
			DescriptorType type;
			VkDescriptorBufferInfo bufferInfo;
			VkDescriptorImageInfo imageInfo;
		};
		void UpdateDescriptorSet(DescriptorSet* descriptorSet, const DescriptorWriteDesc& writeDesc);
		void UpdateInFlightDescriptorSet(InFlightDescriptorSet* descriptorSet, const InFlightDescriptorWriteDesc& writeDesc);
		void SetImageData(Image* image, const void* data, uint32_t size, ImageLayout newLayout);
		void UploadToDeviceLocalBuffer(GpuBuffer* buffer, void* data, uint32_t size);
		void TransitionImageLayout(Image* image, ImageLayout newLayout);

		std::unique_ptr<Swapchain> CreateSwapchain(PresentMode preferredMode);

		VkInstance GetInstance() { return m_Instance; }
		VkDevice GetLogicalDevice() { return m_LogicalDevice; }
		VkPhysicalDevice GetPhysicalDevice() { return m_PhysicalDevice; }
		VkPhysicalDeviceProperties& GetPhysicalDeviceProperties() { return m_PhysicalDeviceProperties; }
		VkCommandPool GetCommandPool() { return m_CommandPool; }

		QueueFamilyIndices GetQueueIndices() { return m_QueueFamilyIndices; }
		VkQueue GetGraphicsQueue() { return m_GraphicsQueue; }
		VkQueue GetPresentQueue() { return m_PresentQueue; }
		uint32_t GetImageCount() { return m_ImageCount; }

		GpuProfiler* GetGpuProfiler() { return m_GpuProfiler.get(); }
		RenderGraph* GetRenderGraph() { return m_RenderGraph.get(); };
		ResourceCache* GetResourceCache() { return m_ResourceCache.get(); }
	private:

		void CreateInstance();
		void CreateDebugUtilsMessenger();
		void FindPhysicalDevice();
		void FindQueueFamilyIndices();
		void CreateLogicalDevice();
		void GetDeviceQueue();
		void CreateCommandPool();
		void CreateSurface(void* windowHandle);

		VkInstance m_Instance;
		VkPhysicalDevice m_PhysicalDevice;
		VkPhysicalDeviceProperties m_PhysicalDeviceProperties;
		VkDevice m_LogicalDevice;
		VkCommandPool m_CommandPool;
		VkSurfaceKHR m_Surface;

		QueueFamilyIndices m_QueueFamilyIndices;
		VkQueue m_GraphicsQueue;
		VkQueue m_PresentQueue;
		uint32_t m_ImageCount;

		std::unique_ptr<GpuProfiler> m_GpuProfiler;
		std::unique_ptr<RenderGraph> m_RenderGraph;
		std::unique_ptr<ResourceCache> m_ResourceCache;

		VkDebugUtilsMessengerEXT m_DebugMessenger;
	};
}