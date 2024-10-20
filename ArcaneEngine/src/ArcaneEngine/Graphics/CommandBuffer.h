#pragma once
#include "VulkanCore/VulkanHandles.h"
#include "VulkanObjects/Attachments.h"
#include "VulkanObjects/DescriptorSet.h"
#include "Common.h"
#include <vector>
#include <optional>

namespace Arc
{
	class CommandBuffer
	{
	public:
		CommandBuffer(DeviceHandle logicalDevice, CommandPoolHandle commandPool);
		~CommandBuffer();

		void Begin();
		void End();

		void SetViewport(const uint32_t area[2]);
		void SetScissors(const uint32_t area[2]);

		void BeginRendering(const std::vector<ColorAttachment>& colorAttachments, std::optional<DepthAttachment> depthAttachment, const uint32_t renderArea[2]);
		//void BeginRendering(const std::vector<ColorAttachment>& colorAttachments, const DepthAttachment& depthAttachment, const uint32_t renderArea[2]);
		//void BeginRendering(const std::vector<ColorAttachment>& colorAttachments, const uint32_t renderArea[2]);
		//void BeginRendering(const DepthAttachment& depthAttachment, const uint32_t renderArea[2]);
		void EndRendering();

		void BindDescriptorSets(PipelineBindPoint bindPoint, PipelineLayoutHandle layout, uint32_t firstSet, const std::vector<DescriptorSetHandle>& descriptorSets);

		void BindPipeline(PipelineHandle pipeline);
		void Draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance);
		void DrawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t vertexOffset, uint32_t firstInstance);
		void BindComputePipeline(PipelineHandle pipeline);
		void Dispatch(uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ);

		void TransitionImage(ImageHandle image, ImageLayout currentLayout, ImageLayout newLayout);
		void ClearColorImage(ImageHandle image, const float clearColor[4], ImageLayout layout);
		void CopyImageToImage(ImageHandle src, const uint32_t srcExtent[3], ImageHandle dst, const uint32_t dstExtent[3]);

		struct ImageBarrier
		{
			ImageHandle Handle;
			ImageLayout OldLayout;
			ImageLayout NewLayout;
		};
		void MemoryBarrier(std::vector<ImageBarrier> imageBarriers);

		CommandBufferHandle GetHandle() { return m_CommandBuffer; }

	private:
		CommandBufferHandle m_CommandBuffer;
	};

}