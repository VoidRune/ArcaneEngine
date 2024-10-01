#include "CommandBuffer.h"
#include "VulkanCore/VulkanHandleCreation.h"
#include "VulkanCore/VulkanLocal.h"
#include <vulkan/vulkan_core.h>

namespace Arc
{
	CommandBuffer::CommandBuffer(DeviceHandle logicalDevice, CommandPoolHandle commandPool)
	{
		CommandBufferCreateInfo commandBufferCreateInfo = {
			.logicalDevice = logicalDevice,
			.commandPool = commandPool
		};

		m_CommandBuffer = CreateCommandBufferHandle(commandBufferCreateInfo);
	}

	CommandBuffer::~CommandBuffer()
	{
		
	}

	void CommandBuffer::Begin()
	{
		VkCommandBufferBeginInfo beginInfo = {};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

		VK_CHECK(vkBeginCommandBuffer((VkCommandBuffer)m_CommandBuffer, &beginInfo));
	}

	void CommandBuffer::End()
	{
		VK_CHECK(vkEndCommandBuffer((VkCommandBuffer)m_CommandBuffer));
	}

	void CommandBuffer::SetViewport(const uint32_t area[2])
	{
		VkViewport viewport = {};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = static_cast<float>(area[0]);
		viewport.height = static_cast<float>(area[1]);
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;
		vkCmdSetViewport((VkCommandBuffer)m_CommandBuffer, 0, 1, &viewport);
	}

	void CommandBuffer::SetScissors(const uint32_t area[2])
	{
		VkRect2D scissor = {};
		scissor.offset = { 0, 0 };
		scissor.extent = { area[0], area[1] };
		vkCmdSetScissor((VkCommandBuffer)m_CommandBuffer, 0, 1, &scissor);
	}

	void CommandBuffer::BeginRendering(const std::vector<ColorAttachment>& colorAttachments, std::optional<DepthAttachment> depthAttachment, const uint32_t renderArea[2])
	{
		std::vector<VkRenderingAttachmentInfo> colorInfo(colorAttachments.size());
		for (size_t i = 0; i < colorInfo.size(); i++)
		{
			auto& info = colorInfo[i];
			auto& colorAttachment = colorAttachments[i];
			info = {};
			info.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
			info.imageView = (VkImageView)colorAttachment.ImageView;
			info.imageLayout = (VkImageLayout)colorAttachment.ImageLayout;
			info.loadOp = (VkAttachmentLoadOp)colorAttachment.LoadOp;
			info.storeOp = (VkAttachmentStoreOp)colorAttachment.StoreOp;
			info.clearValue = { colorAttachment.ClearColor[0], colorAttachment.ClearColor[1], colorAttachment.ClearColor[2], colorAttachment.ClearColor[3] };
		}

		VkRenderingInfo renderingInfo = {};
		renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
		renderingInfo.renderArea = VkRect2D{ {0, 0}, { renderArea[0], renderArea[1] } };
		renderingInfo.layerCount = 1;
		renderingInfo.colorAttachmentCount = colorInfo.size();
		renderingInfo.pColorAttachments = colorInfo.data();
		if (depthAttachment.has_value())
		{
			auto& da = depthAttachment.value();
			VkRenderingAttachmentInfo depthInfo = {};
			depthInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
			depthInfo.imageView = (VkImageView)da.ImageView;
			depthInfo.imageLayout = (VkImageLayout)da.ImageLayout;
			depthInfo.loadOp = (VkAttachmentLoadOp)da.LoadOp;
			depthInfo.storeOp = (VkAttachmentStoreOp)da.StoreOp;
			depthInfo.clearValue.depthStencil = { da.ClearValue, 0 };
			renderingInfo.pDepthAttachment = &depthInfo;
		}

		vkCmdBeginRendering((VkCommandBuffer)m_CommandBuffer, &renderingInfo);
	}
	/*
	void CommandBuffer::BeginRendering(const std::vector<ColorAttachment>& colorAttachments, const DepthAttachment& depthAttachment, const uint32_t renderArea[2])
	{
		std::vector<VkRenderingAttachmentInfo> colorInfo(colorAttachments.size());
		for (size_t i = 0; i < colorInfo.size(); i++)
		{
			auto& info = colorInfo[i];
			auto& colorAttachment = colorAttachments[i];
			info = {};
			info.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
			info.imageView = (VkImageView)colorAttachment.ImageView;
			info.imageLayout = (VkImageLayout)colorAttachment.ImageLayout;
			info.loadOp = (VkAttachmentLoadOp)colorAttachment.LoadOp;
			info.storeOp = (VkAttachmentStoreOp)colorAttachment.StoreOp;
			info.clearValue = { colorAttachment.ClearColor[0], colorAttachment.ClearColor[1], colorAttachment.ClearColor[2], colorAttachment.ClearColor[3] };
		}

		VkRenderingAttachmentInfo depthInfo = {};
		depthInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
		depthInfo.imageView = (VkImageView)depthAttachment.ImageView;
		depthInfo.imageLayout = (VkImageLayout)depthAttachment.ImageLayout;
		depthInfo.loadOp = (VkAttachmentLoadOp)depthAttachment.LoadOp;
		depthInfo.storeOp = (VkAttachmentStoreOp)depthAttachment.StoreOp;
		depthInfo.clearValue.depthStencil = { depthAttachment.ClearValue, 0 };

		VkRenderingInfo renderingInfo = {};
		renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
		renderingInfo.renderArea = VkRect2D{ {0, 0}, { renderArea[0], renderArea[1] } };
		renderingInfo.layerCount = 1;
		renderingInfo.colorAttachmentCount = colorInfo.size();
		renderingInfo.pColorAttachments = colorInfo.data();
		renderingInfo.pDepthAttachment = &depthInfo;

		vkCmdBeginRendering((VkCommandBuffer)m_CommandBuffer, &renderingInfo);
	}

	void CommandBuffer::BeginRendering(const std::vector<ColorAttachment>& colorAttachments, const uint32_t renderArea[2])
	{
		std::vector<VkRenderingAttachmentInfo> colorInfo(colorAttachments.size());
		for (size_t i = 0; i < colorInfo.size(); i++)
		{
			auto& info = colorInfo[i];
			auto& colorAttachment = colorAttachments[i];
			info = {};
			info.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
			info.imageView = (VkImageView)colorAttachment.ImageView;
			info.imageLayout = (VkImageLayout)colorAttachment.ImageLayout;
			info.loadOp = (VkAttachmentLoadOp)colorAttachment.LoadOp;
			info.storeOp = (VkAttachmentStoreOp)colorAttachment.StoreOp;
			info.clearValue = { colorAttachment.ClearColor[0], colorAttachment.ClearColor[1], colorAttachment.ClearColor[2], colorAttachment.ClearColor[3] };
		}

		VkRenderingInfo renderingInfo = {};
		renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
		renderingInfo.renderArea = VkRect2D{ {0, 0}, { renderArea[0], renderArea[1] }};
		renderingInfo.layerCount = 1;
		renderingInfo.colorAttachmentCount = colorInfo.size();
		renderingInfo.pColorAttachments = colorInfo.data();

		vkCmdBeginRendering((VkCommandBuffer)m_CommandBuffer, &renderingInfo);
	}

	void CommandBuffer::BeginRendering(const DepthAttachment& depthAttachment, const uint32_t renderArea[2])
	{
		VkRenderingAttachmentInfo depthInfo = {};
		depthInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
		depthInfo.imageView = (VkImageView)depthAttachment.ImageView;
		depthInfo.imageLayout = (VkImageLayout)depthAttachment.ImageLayout;
		depthInfo.loadOp = (VkAttachmentLoadOp)depthAttachment.LoadOp;
		depthInfo.storeOp = (VkAttachmentStoreOp)depthAttachment.StoreOp;
		depthInfo.clearValue.depthStencil = { depthAttachment.ClearValue, 0 };

		VkRenderingInfo renderingInfo = {};
		renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
		renderingInfo.renderArea = VkRect2D{ {0, 0}, { renderArea[0], renderArea[1] } };
		renderingInfo.layerCount = 1;
		renderingInfo.pDepthAttachment = &depthInfo;

		vkCmdBeginRendering((VkCommandBuffer)m_CommandBuffer, &renderingInfo);
	}
	*/
	void CommandBuffer::EndRendering()
	{
		vkCmdEndRendering((VkCommandBuffer)m_CommandBuffer);
	}

	void CommandBuffer::BindDescriptorSets(PipelineBindPoint bindPoint, PipelineLayoutHandle layout, uint32_t firstSet, const std::vector<DescriptorSetHandle>& descriptorSets)
	{
		std::vector<VkDescriptorSet> sets(descriptorSets.size());
		for (size_t i = 0; i < descriptorSets.size(); i++)
		{
			sets[i] = (VkDescriptorSet)descriptorSets[i];
		}

		vkCmdBindDescriptorSets((VkCommandBuffer)m_CommandBuffer, static_cast<VkPipelineBindPoint>(bindPoint), (VkPipelineLayout)layout, firstSet, sets.size(), sets.data(), 0, nullptr);
	}

	void CommandBuffer::BindPipeline(PipelineHandle pipeline)
	{
		vkCmdBindPipeline((VkCommandBuffer)m_CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, (VkPipeline)pipeline);
	}

	void CommandBuffer::BindComputePipeline(PipelineHandle pipeline)
	{
		vkCmdBindPipeline((VkCommandBuffer)m_CommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, (VkPipeline)pipeline);
	}

	void CommandBuffer::Draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance)
	{
		vkCmdDraw((VkCommandBuffer)m_CommandBuffer, vertexCount, instanceCount, firstVertex, firstInstance);
	}

	void CommandBuffer::DrawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t vertexOffset, uint32_t firstInstance)
	{
		vkCmdDrawIndexed((VkCommandBuffer)m_CommandBuffer, indexCount, instanceCount, firstVertex, vertexOffset, firstInstance);
	}

	void CommandBuffer::Dispatch(uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ)
	{
		vkCmdDispatch((VkCommandBuffer)m_CommandBuffer, groupCountX, groupCountY, groupCountZ);
	}

	void CommandBuffer::TransitionImage(ImageHandle image, ImageLayout currentLayout, ImageLayout newLayout)
	{
		VkImageMemoryBarrier2 imageBarrier = {};
		imageBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
		imageBarrier.pNext = nullptr;

		imageBarrier.srcStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
		imageBarrier.srcAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT;
		imageBarrier.dstStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
		imageBarrier.dstAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT | VK_ACCESS_2_MEMORY_READ_BIT;

		imageBarrier.oldLayout = (VkImageLayout)currentLayout;
		imageBarrier.newLayout = (VkImageLayout)newLayout;

		VkImageAspectFlags aspectMask = (newLayout == ImageLayout::DepthAttachmentOptimal) ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
		VkImageSubresourceRange subRange{};
		subRange.aspectMask = aspectMask;
		subRange.baseMipLevel = 0;
		subRange.levelCount = VK_REMAINING_MIP_LEVELS;
		subRange.baseArrayLayer = 0;
		subRange.layerCount = VK_REMAINING_ARRAY_LAYERS;

		imageBarrier.subresourceRange = subRange;
		imageBarrier.image = (VkImage)image;

		VkDependencyInfo depInfo{};
		depInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
		depInfo.pNext = nullptr;

		depInfo.imageMemoryBarrierCount = 1;
		depInfo.pImageMemoryBarriers = &imageBarrier;

		vkCmdPipelineBarrier2((VkCommandBuffer)m_CommandBuffer, &depInfo);
	}

	void CommandBuffer::ClearColorImage(ImageHandle image, const float clearColor[4], ImageLayout layout)
	{
		VkClearColorValue clearValue;
		clearValue = { {clearColor[0], clearColor[1], clearColor[2], clearColor[3]}};

		VkImageSubresourceRange subImage{};
		subImage.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		subImage.baseMipLevel = 0;
		subImage.levelCount = VK_REMAINING_MIP_LEVELS;
		subImage.baseArrayLayer = 0;
		subImage.layerCount = VK_REMAINING_ARRAY_LAYERS;

		vkCmdClearColorImage((VkCommandBuffer)m_CommandBuffer, (VkImage)image, (VkImageLayout)layout, &clearValue, 1, &subImage);
	}

	void CommandBuffer::CopyImageToImage(ImageHandle src, const uint32_t srcExtent[3], ImageHandle dst, const uint32_t dstExtent[3])
	{
		VkImageBlit2 blitRegion = {};
		blitRegion.sType = VK_STRUCTURE_TYPE_IMAGE_BLIT_2;
		blitRegion.pNext = nullptr;

		blitRegion.srcOffsets[1].x = srcExtent[0];
		blitRegion.srcOffsets[1].y = srcExtent[1];
		blitRegion.srcOffsets[1].z = srcExtent[2];

		blitRegion.dstOffsets[1].x = dstExtent[0];
		blitRegion.dstOffsets[1].y = dstExtent[1];
		blitRegion.dstOffsets[1].z = dstExtent[2];

		blitRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		blitRegion.srcSubresource.baseArrayLayer = 0;
		blitRegion.srcSubresource.layerCount = 1;
		blitRegion.srcSubresource.mipLevel = 0;

		blitRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		blitRegion.dstSubresource.baseArrayLayer = 0;
		blitRegion.dstSubresource.layerCount = 1;
		blitRegion.dstSubresource.mipLevel = 0;

		VkBlitImageInfo2 blitInfo{ .sType = VK_STRUCTURE_TYPE_BLIT_IMAGE_INFO_2, .pNext = nullptr };
		blitInfo.dstImage = (VkImage)dst;
		blitInfo.dstImageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		blitInfo.srcImage = (VkImage)src;
		blitInfo.srcImageLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		blitInfo.filter = VK_FILTER_LINEAR;
		blitInfo.regionCount = 1;
		blitInfo.pRegions = &blitRegion;

		vkCmdBlitImage2((VkCommandBuffer)m_CommandBuffer, &blitInfo);
	}
}