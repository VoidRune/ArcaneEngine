#pragma once
#include "VulkanCore/VulkanHandles.h"
#include "CommandBuffer.h"
#include "PresentQueue.h"
#include <functional>

namespace Arc
{

	struct Resource
	{

	};

	struct RenderPass
	{
		std::vector<ColorAttachment> ColorAttachments = {};
		std::optional<DepthAttachment> DepthAttachment = {};
		std::function<void(CommandBuffer* cb, uint32_t frameIndex)> ExecuteFunction = nullptr;

		std::vector<Resource> Inputs = {};
		std::vector<Resource> Outputs = {};
	};

	struct PresentPass
	{
		AttachmentLoadOp LoadOp = AttachmentLoadOp::DontCare;
		std::array<float, 4> ClearColor = {};
		std::function<void(CommandBuffer* cb, uint32_t frameIndex)> ExecuteFunction = nullptr;

		std::vector<Resource> Inputs = {};
	};

	class RenderGraph
	{
	public:
		RenderGraph();
		~RenderGraph();

		void AddPass(const RenderPass& renderPass);
		void SetPresentPass(const PresentPass& presentPass);
		void BuildGraph();
		void Execute(FrameData frameData, const uint32_t extent[2]);
	private:
		std::vector<RenderPass> m_BuildRenderPasses;
		PresentPass m_BuildPresentPass;

		std::vector<RenderPass> m_RenderPasses;
		PresentPass m_PresentPass;
	};
}