#include "RenderPassCache.h"

namespace Arc
{
	RenderPassCache::RenderPassCache(VkDevice device)
	{
		m_Device = device;
	}

	RenderPassCache::~RenderPassCache()
	{
		Clear();
	}

	void RenderPassCache::Clear()
	{
		for (int i = 0; i < m_RenderPasses.size(); i++)
		{
			vkDestroyRenderPass(m_Device, m_RenderPasses[i], nullptr);
		}

		for (size_t i = 0; i < m_Framebuffers.size(); i++)
		{
			vkDestroyFramebuffer(m_Device, m_Framebuffers[i], nullptr);
		}

		m_RenderPasses.clear();
		m_Framebuffers.clear();
	}

	uint32_t RenderPassCache::CreateRenderPass(const RenderPassDesc& desc, VkImageLayout colorLayout)
	{
		VkRenderPass renderPass;

		uint32_t currAttachmentIndex = 0;
		std::vector<VkAttachmentDescription> attachmentsDesc;
		std::vector<VkAttachmentReference> colorRef;

		for (auto colorAttachmentDesc : desc.m_ColorAttachments)
		{
			VkAttachmentDescription attachmentDesc = {};
			attachmentDesc.format = colorAttachmentDesc.proxy.format;
			attachmentDesc.samples = VK_SAMPLE_COUNT_1_BIT;
			attachmentDesc.loadOp = static_cast<VkAttachmentLoadOp>(colorAttachmentDesc.loadOp);
			attachmentDesc.storeOp = static_cast<VkAttachmentStoreOp>(colorAttachmentDesc.storeOp);
			attachmentDesc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			attachmentDesc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			attachmentDesc.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			attachmentDesc.finalLayout = colorLayout;
			attachmentsDesc.push_back(attachmentDesc);

			VkAttachmentReference attachmentRef = {};
			attachmentRef.attachment = currAttachmentIndex++;
			attachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
			colorRef.push_back(attachmentRef);
		}

		std::array<VkSubpassDescription, 1> subpass = {};
		subpass[0].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass[0].colorAttachmentCount = colorRef.size();
		subpass[0].pColorAttachments = colorRef.data();

		//subpass[1].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		//subpass[1].colorAttachmentCount = colorRef.size();
		//subpass[1].pColorAttachments = colorRef.data();

		VkAttachmentReference depthRef = {};

		if (desc.m_DepthAttachment.proxy.format != VK_FORMAT_UNDEFINED)
		{
			VkAttachmentDescription depthAttachmentTemp = {};
			depthAttachmentTemp.format = desc.m_DepthAttachment.proxy.format;
			depthAttachmentTemp.samples = VK_SAMPLE_COUNT_1_BIT;
			depthAttachmentTemp.loadOp = static_cast<VkAttachmentLoadOp>(desc.m_DepthAttachment.loadOp);
			depthAttachmentTemp.storeOp = static_cast<VkAttachmentStoreOp>(desc.m_DepthAttachment.storeOp);
			depthAttachmentTemp.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			depthAttachmentTemp.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			depthAttachmentTemp.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			depthAttachmentTemp.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
			attachmentsDesc.push_back(depthAttachmentTemp);

			depthRef.attachment = currAttachmentIndex++;
			depthRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
			subpass[0].pDepthStencilAttachment = &depthRef;
			//subpass[1].pDepthStencilAttachment = &depthRef;
		}

		std::array<VkSubpassDependency, 1> dependency = {};
		dependency[0].srcSubpass = VK_SUBPASS_EXTERNAL;
		dependency[0].dstSubpass = 0;
		dependency[0].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		dependency[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		dependency[0].srcAccessMask = 0;
		dependency[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

		VkRenderPassCreateInfo renderPassInfo = {};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderPassInfo.attachmentCount = attachmentsDesc.size();
		renderPassInfo.pAttachments = attachmentsDesc.data();
		renderPassInfo.subpassCount = subpass.size();
		renderPassInfo.pSubpasses = subpass.data();
		renderPassInfo.dependencyCount = dependency.size();
		renderPassInfo.pDependencies = dependency.data();

		VK_CHECK(vkCreateRenderPass(m_Device, &renderPassInfo, nullptr, &renderPass));

		uint32_t index = m_RenderPasses.size();
		m_RenderPasses.push_back(renderPass);
		return index;
	}

	uint32_t RenderPassCache::CreateFramebuffer(VkRenderPass renderPass, const std::vector<VkImageView>& imageViews, VkExtent2D extent)
	{
		VkFramebuffer framebuffer;

		VkFramebufferCreateInfo framebufferInfo = {};
		framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass = renderPass;
		framebufferInfo.attachmentCount = imageViews.size();
		framebufferInfo.pAttachments = imageViews.data();
		framebufferInfo.width = extent.width;
		framebufferInfo.height = extent.height;
		framebufferInfo.layers = 1;

		VK_CHECK(vkCreateFramebuffer(m_Device, &framebufferInfo, nullptr, &framebuffer));

		uint32_t index = m_Framebuffers.size();
		m_Framebuffers.push_back(framebuffer);
		return index;
	}

}