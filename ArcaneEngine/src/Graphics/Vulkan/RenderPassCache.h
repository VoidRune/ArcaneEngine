#pragma once
#include "VkLocal.h"
#include "RenderPass.h"

namespace Arc
{
	class RenderPassCache
	{
	public:
		RenderPassCache(VkDevice device);
		~RenderPassCache();

		uint32_t CreateRenderPass(const RenderPassDesc& desc, VkImageLayout colorLayout);
		uint32_t CreateFramebuffer(VkRenderPass renderPass, const std::vector<VkImageView>& imageViews, VkExtent2D extent);

		VkRenderPass GetRenderPass(uint32_t index) { return m_RenderPasses[index]; }
		VkFramebuffer GetFramebuffer(uint32_t index) { return m_Framebuffers[index]; }

		void Clear();

	private:
		VkDevice m_Device;

		std::vector<VkRenderPass> m_RenderPasses;
		std::vector<VkFramebuffer> m_Framebuffers;
	};

}