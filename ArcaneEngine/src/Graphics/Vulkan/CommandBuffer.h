#pragma once
#include "VkLocal.h"
#include "Common.h"

namespace Arc
{
	class CommandBuffer;
	class FrameData
	{
	public:
		CommandBuffer* CommandBuffer;
		uint32_t FrameIndex;
	private:
		uint32_t m_SwapchainImageIndex;

		friend class PresentQueue;
		friend class RenderGraph;
	};

	class CommandBuffer
	{
	public:
		CommandBuffer();
		~CommandBuffer();
		void Create(VkDevice device, VkCommandPool commandPool);
		void Begin();
		void End();

		void SetViewport(VkExtent2D extent);
		void SetScissor(VkExtent2D extent);
		void SetDepthBias(float depthBiasConstantFactor, float depthBiasClamp, float depthBiasSlopeFactor);

		void BeginRenderPass(VkRenderPass renderPass, VkFramebuffer framebuffer, VkExtent2D renderArea, const std::vector<VkClearValue>& clearValues);
		void EndRenderPass();

		void PushConstants(VkPipelineLayout layout, ShaderStage stageFlags, uint32_t size, const void* data);
		void BindDescriptorSets(PipelineBindPoint bindPoint, VkPipelineLayout layout, uint32_t firstSet, std::vector<VkDescriptorSet> descriptorSets);
		void BindPipeline(PipelineBindPoint bindPoint, VkPipeline pipeline);
		void BindVertexBuffers(std::vector<VkBuffer> buffers);
		void BindIndexBuffer(VkBuffer buffer, IndexType indexType);
		void Dispatch(uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ);
		void Draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance);
		void DrawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t vertexOffset, uint32_t firstInstance);

		void PipelineBarrier(AccessFlags srcAccess, AccessFlags dstAccess, PipelineStage srcStage, PipelineStage dstStage);
		
		struct ImageTransitionInfo
		{
			VkImage image;
			VkImageLayout oldLayout;
			VkImageLayout newLayout;
			AccessFlags srcAccessMask;
			AccessFlags dstAccessMask;
			ImageAspect aspect;
		};
		void ImageTransition(const ImageTransitionInfo& imageTransitionInfo, PipelineStage srcStage, PipelineStage dstStage);

		VkCommandBuffer GetHandle() { return m_CommandBuffer; }
	private:
		VkDevice m_Device;
		VkCommandBuffer m_CommandBuffer;

	};
}