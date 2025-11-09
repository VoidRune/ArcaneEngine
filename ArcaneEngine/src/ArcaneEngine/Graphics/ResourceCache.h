#pragma once
#include "VulkanCore/VulkanHandles.h"
#include "VulkanObjects/GpuBuffer.h"
#include "VulkanObjects/GpuImage.h"
#include "VulkanObjects/Sampler.h"
#include "VulkanObjects/Shader.h"
#include "VulkanObjects/DescriptorSet.h"
#include "VulkanObjects/Pipeline.h"
#include "VulkanObjects/ComputePipeline.h"
#include "VulkanObjects/BottomLevelAS.h"
#include "VulkanObjects/TopLevelAS.h"
#include "VulkanObjects/RayTracingPipeline.h"
#include <unordered_map>

namespace Arc
{
	class Device;
	class ResourceCache
	{
	public:
		ResourceCache(Device* device);
		~ResourceCache();

		void* MapMemory(GpuBuffer* buffer);
		void UnmapMemory(GpuBuffer* buffer);
		void* MapMemory(GpuBufferArray* bufferArray, uint32_t frameIndex);
		void UnmapMemory(GpuBufferArray* bufferArray, uint32_t frameIndex);

		void CreateGpuBuffer(GpuBuffer* gpuBuffer, const GpuBufferDesc& desc);
		void CreateGpuBufferArray(GpuBufferArray* gpuBufferArray, const GpuBufferDesc& desc);
		void CreateGpuImage(GpuImage* gpuImage, const GpuImageDesc& desc);
		void CreateSampler(Sampler* sampler, const SamplerDesc& desc);
		void CreateShader(Shader* shader, const ShaderDesc& desc);
		void AllocateDescriptorSet(DescriptorSet* descriptor, const DescriptorSetDesc& desc);
		void AllocateDescriptorSetArray(DescriptorSetArray* descriptorArray, const DescriptorSetDesc& desc);
		void CreatePipeline(Pipeline* pipeline, const PipelineDesc& desc);
		void CreateComputePipeline(ComputePipeline* pipeline, const ComputePipelineDesc& desc);
		void CreateBottomLevelAS(BottomLevelAS* accelerationStructure, const BottomLevelASDesc& desc);
		void CreateTopLevelAS(TopLevelAS* accelerationStructure, const TopLevelASDesc& desc);
		void CreateRayTracingPipeline(RayTracingPipeline* pipeline, const RayTracingPipelineDesc& desc);

		void ReleaseResource(GpuBuffer* gpuBuffer);
		void ReleaseResource(GpuBufferArray* gpuBufferArray);
		void ReleaseResource(GpuImage* gpuImage);
		void ReleaseResource(Sampler* sampler);
		void ReleaseResource(Shader* shader);
		void ReleaseResource(Pipeline* pipeline);
		void ReleaseResource(ComputePipeline* computePipeline);
		void ReleaseResource(BottomLevelAS* bottomLevelAS);
		void ReleaseResource(TopLevelAS* topLevelAS);
		void ReleaseResource(RayTracingPipeline* raytracingPipeline);

		void FreeResources();
		void PrintHeapBudgets();

	private:
		Device* m_Device;
		DeviceHandle m_LogicalDevice;
		PhysicalDeviceHandle m_PhysicalDevice;
		QueueHandle m_GraphicsQueue;
		CommandPoolHandle m_CommandPool;
		AllocatorHandle m_Allocator;
		DescriptorPoolHandle m_DescriptorPool;
		uint32_t m_FramesInFlight;

		enum class ResourceType
		{
			GpuBuffer,
			GpuBufferArray,
			GpuImage,
			Sampler,
			Shader,
			Pipeline,
			ComputePipeline,
			BottomLevelAS,
			TopLevelAS,
			RayTracingPipeline
		};

		std::unordered_map<uint64_t, DescriptorSetLayoutHandle> m_DescriptorSetLayouts;
		std::unordered_map<void*, ResourceType> m_Resources;
	};
}