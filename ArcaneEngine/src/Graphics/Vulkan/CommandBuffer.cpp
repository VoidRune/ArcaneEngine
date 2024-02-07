#include "CommandBuffer.h"

namespace Arc
{
	CommandBuffer::CommandBuffer()
	{

	}

	CommandBuffer::~CommandBuffer()
	{

	}

	void CommandBuffer::Create(VkDevice device, VkCommandPool commandPool)
	{
		m_Device = device;

		VkCommandBufferAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandPool = commandPool;
		allocInfo.commandBufferCount = static_cast<uint32_t>(1);

		VK_CHECK(vkAllocateCommandBuffers(m_Device, &allocInfo, &m_CommandBuffer));
	}

	void CommandBuffer::Begin()
	{
		VkCommandBufferBeginInfo beginInfo = {};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

		VK_CHECK(vkBeginCommandBuffer(m_CommandBuffer, &beginInfo));
	}

	void CommandBuffer::End()
	{
		VK_CHECK(vkEndCommandBuffer(m_CommandBuffer));
	}

	void CommandBuffer::SetViewport(VkExtent2D extent)
	{
		VkViewport viewport = {};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = static_cast<float>(extent.width);
		viewport.height = static_cast<float>(extent.height);
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;
		vkCmdSetViewport(m_CommandBuffer, 0, 1, &viewport);
	}

	void CommandBuffer::SetScissor(VkExtent2D extent)
	{
		VkRect2D scissor = {};
		scissor.offset = { 0, 0 };
		scissor.extent = extent;
		vkCmdSetScissor(m_CommandBuffer, 0, 1, &scissor);
	}

	void CommandBuffer::SetDepthBias(float depthBiasConstantFactor, float depthBiasClamp, float depthBiasSlopeFactor)
	{
		vkCmdSetDepthBias(m_CommandBuffer, depthBiasConstantFactor, depthBiasClamp, depthBiasSlopeFactor);
	}

	void CommandBuffer::BeginRenderPass(VkRenderPass renderPass, VkFramebuffer framebuffer, VkExtent2D renderArea, const std::vector<VkClearValue>& clearValues)
	{
		VkRenderPassBeginInfo renderPassBeginInfo = {};
		renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassBeginInfo.renderPass = renderPass;
		renderPassBeginInfo.framebuffer = framebuffer;

		renderPassBeginInfo.renderArea.offset = { 0, 0 };
		renderPassBeginInfo.renderArea.extent = renderArea;
		renderPassBeginInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
		renderPassBeginInfo.pClearValues = clearValues.data();

		vkCmdBeginRenderPass(m_CommandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
	}

	void CommandBuffer::EndRenderPass()
	{
		vkCmdEndRenderPass(m_CommandBuffer);
	}

	void CommandBuffer::PushConstants(VkPipelineLayout layout, ShaderStage stageFlags, uint32_t size, const void* data)
	{
		vkCmdPushConstants(m_CommandBuffer, layout, static_cast<VkShaderStageFlags>(stageFlags), 0, size, data);
	}

	void CommandBuffer::BindDescriptorSets(PipelineBindPoint bindPoint, VkPipelineLayout layout, uint32_t firstSet, std::vector<VkDescriptorSet> descriptorSets)
	{
		vkCmdBindDescriptorSets(m_CommandBuffer, static_cast<VkPipelineBindPoint>(bindPoint), layout, firstSet, descriptorSets.size(), descriptorSets.data(), 0, nullptr);
	}

	void CommandBuffer::BindPipeline(PipelineBindPoint bindPoint, VkPipeline pipeline)
	{
		vkCmdBindPipeline(m_CommandBuffer, static_cast<VkPipelineBindPoint>(bindPoint), pipeline);
	}

	void CommandBuffer::BindVertexBuffers(std::vector<VkBuffer> buffers)
	{
		VkDeviceSize offsets[] = { 0, 0, 0, 0 };
		vkCmdBindVertexBuffers(m_CommandBuffer, 0, buffers.size(), buffers.data(), offsets);
	}

	void CommandBuffer::BindIndexBuffer(VkBuffer buffer, IndexType indexType)
	{
		vkCmdBindIndexBuffer(m_CommandBuffer, buffer, 0, static_cast<VkIndexType>(indexType));
	}

	void CommandBuffer::Dispatch(uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ)
	{
		vkCmdDispatch(m_CommandBuffer, groupCountX, groupCountY, groupCountZ);
	}

	void CommandBuffer::Draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance)
	{
		vkCmdDraw(m_CommandBuffer, vertexCount, instanceCount, firstVertex, firstInstance);
	}

	void CommandBuffer::DrawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t vertexOffset, uint32_t firstInstance)
	{
		vkCmdDrawIndexed(m_CommandBuffer, indexCount, instanceCount, firstVertex, vertexOffset, firstInstance);
	}

	void CommandBuffer::PipelineBarrier(AccessFlags srcAccess, AccessFlags dstAccess, PipelineStage srcStage, PipelineStage dstStage)
	{
		VkMemoryBarrier memBarrier;
		memBarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
		memBarrier.pNext = nullptr;
		memBarrier.srcAccessMask = static_cast<VkAccessFlagBits>(srcAccess);
		memBarrier.dstAccessMask = static_cast<VkAccessFlagBits>(dstAccess);
		vkCmdPipelineBarrier(m_CommandBuffer,
			static_cast<VkPipelineStageFlags>(srcStage),
			static_cast<VkPipelineStageFlags>(dstStage),
			0,
			1, &memBarrier,
			0, nullptr,
			0, nullptr);
	}

	void CommandBuffer::ImageTransition(const ImageTransitionInfo& imageTransitionInfo, PipelineStage srcStage, PipelineStage dstStage)
	{
		VkImageMemoryBarrier barrier{};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.pNext = nullptr;
		barrier.srcAccessMask = static_cast<VkAccessFlags>(imageTransitionInfo.srcAccessMask);
		barrier.dstAccessMask = static_cast<VkAccessFlags>(imageTransitionInfo.dstAccessMask);
		barrier.oldLayout = static_cast<VkImageLayout>(imageTransitionInfo.oldLayout);
		barrier.newLayout = static_cast<VkImageLayout>(imageTransitionInfo.newLayout);
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.image = imageTransitionInfo.image;
		barrier.subresourceRange.aspectMask = static_cast<VkImageAspectFlags>(imageTransitionInfo.aspect);
		barrier.subresourceRange.baseMipLevel = 0;
		barrier.subresourceRange.levelCount = 1;
		barrier.subresourceRange.baseArrayLayer = 0;
		barrier.subresourceRange.layerCount = 1;
		
		vkCmdPipelineBarrier(m_CommandBuffer,
			static_cast<VkPipelineStageFlags>(srcStage),
			static_cast<VkPipelineStageFlags>(dstStage),
			0,				/* Dependency flags */
			0, nullptr,		/* Memory barrier*/
			0, nullptr,		/* Buffer memory barrier */
			1, &barrier);	/* Image memory barrier */
	}

}