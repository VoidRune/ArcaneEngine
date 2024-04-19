#pragma once
#include "Vulkan/VkLocal.h"
#include "Vulkan/Common.h"
#include "Vulkan/GpuBuffer.h"
#include "Vulkan/Image.h"
#include "Vulkan/DescriptorSet.h"
#include "Vulkan/Shader.h"
#include "Vulkan/Pipeline.h"
#include "Vulkan/vk_mem_alloc.h"

namespace Arc
{
	class ResourceCache
	{
	public:
		ResourceCache(VkInstance instance, VkDevice device, VkPhysicalDevice physicalDevice, uint32_t imageCount);
		~ResourceCache();

		void CreateBuffer(GpuBuffer* buffer, const GpuBufferDesc& bufferDescription);
		void CreateBufferSet(GpuBufferSet* buffer, const GpuBufferDesc& bufferDescription);
		void CreateImage(GpuImage* image, const GpuImageDesc& imageDescription);
		void CreateSampler(Sampler* sampler, const SamplerDesc& samplerDescription);
		void AllocateDescriptorSets(const std::vector<DescriptorSet*> descriptorSet, const DescriptorSetLayoutDesc& layoutDescription);
		void AllocateDescriptorSetArray(DescriptorSetArray* frameDescriptorSet, const DescriptorSetLayoutDesc& layoutDescription);
		void CreateShader(Shader* shader, const ShaderDesc& shaderDesc);
		void CreatePipeline(Pipeline* pipeline, const PipelineDesc& pipelineDesc);
		void CreateComputePipeline(ComputePipeline* pipeline, const ComputePipelineDesc& pipelineDesc);

		void* MapMemory(GpuBuffer* buffer);
		void UnmapMemory(GpuBuffer* buffer);
		void* MapMemory(GpuBufferSet* buffer, uint32_t frameIndex);
		void UnmapMemory(GpuBufferSet* buffer, uint32_t frameIndex);

		void ReleaseResource(GpuBuffer* buffer);
		void ReleaseResource(GpuBufferSet* buffer);
		void ReleaseResource(GpuImage* image);
		void ReleaseResource(Sampler* sampler);
		void ReleaseResource(Shader* shader);
		void ReleaseResource(Pipeline* pipeline);
		void ReleaseResource(ComputePipeline* pipeline);
		void FreeResources();

		void PrintHeapBudgets();

		VmaAllocator GetAllocator() { return m_MemoryAllocator; }
		VkDescriptorPool GetDescriptorPool() { return m_DescriptorPool; }
	private:

		VkDescriptorSetLayout GetDescriptorSetLayout(const std::vector<VkDescriptorSetLayoutBinding>& layoutBindings, uint32_t flags);

		VkDevice m_Device;
		VkPhysicalDevice m_PhysicalDevice;
		VkDescriptorPool m_DescriptorPool;
		uint32_t m_ImageCount;
		VmaAllocator m_MemoryAllocator;

		std::unordered_map<uint64_t, VkDescriptorSetLayout> m_DescriptorSetLayouts;
		std::unordered_map<void*, std::function<void()>> m_ResourceReleaseFuctions;
	};
}