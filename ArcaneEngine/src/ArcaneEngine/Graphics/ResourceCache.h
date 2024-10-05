#pragma once
#include "VulkanCore/VulkanHandles.h"
#include "VulkanObjects/GpuBuffer.h"
#include "VulkanObjects/GpuImage.h"
#include "VulkanObjects/Sampler.h"
#include "VulkanObjects/Shader.h"
#include "VulkanObjects/DescriptorSet.h"
#include "VulkanObjects/Pipeline.h"
#include "VulkanObjects/ComputePipeline.h"
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

		void ReleaseResource(GpuBuffer* gpuBuffer);
		void ReleaseResource(GpuBufferArray* gpuBufferArray);
		void ReleaseResource(GpuImage* gpuImage);
		void ReleaseResource(Sampler* sampler);
		void ReleaseResource(Shader* shader);
		void ReleaseResource(Pipeline* pipeline);
		void ReleaseResource(ComputePipeline* computePipeline);

		void FreeResources();
		void PrintHeapBudgets();

	private:
		DeviceHandle m_LogicalDevice;
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
			ComputePipeline
		};

		std::unordered_map<uint64_t, DescriptorSetLayoutHandle> m_DescriptorSetLayouts;
		std::unordered_map<void*, ResourceType> m_Resources;
	};
}