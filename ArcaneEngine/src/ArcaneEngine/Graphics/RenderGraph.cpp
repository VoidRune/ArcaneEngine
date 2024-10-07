#include "RenderGraph.h"
#include "ArcaneEngine/Core/Log.h"

namespace Arc
{
	RenderGraph::RenderGraph()
	{

	}

	RenderGraph::~RenderGraph()
	{

	}

	void RenderGraph::AddPass(const RenderPass& renderPass)
	{
		m_BuildRenderPasses.push_back(renderPass);
	}

	void RenderGraph::SetPresentPass(const PresentPass& presentPass)
	{
		m_BuildPresentPass = presentPass;
	}

	void RenderGraph::BuildGraph()
	{
		m_RenderPasses.clear();

		{
			m_RenderPasses = m_BuildRenderPasses;
		}

		{
			m_PresentPass = m_BuildPresentPass;
			if (m_PresentPass.ExecuteFunction == nullptr)
			{
				ARC_LOG_WARNING("RenderGraph - PresentPass was not set during graph setup!");
			}
		}

		m_BuildRenderPasses.clear();
		m_BuildPresentPass = {};
	}

	void RenderGraph::Execute(FrameData frameData, const uint32_t extent[2])
	{
		auto& cmd = frameData.CommandBuffer;

		cmd->SetViewport(extent);
		cmd->SetScissors(extent);

		for (auto& pass : m_RenderPasses)
		{
			bool hasAttachments = pass.ColorAttachments.size() != 0 && pass.DepthAttachment.has_value();
			if (hasAttachments)
				cmd->BeginRendering(pass.ColorAttachments, pass.DepthAttachment, extent);
			pass.ExecuteFunction(frameData.CommandBuffer, frameData.FrameIndex);
			if (hasAttachments)
				cmd->EndRendering();
		}

		cmd->TransitionImage(frameData.PresentImage, Arc::ImageLayout::Undefined, Arc::ImageLayout::TransferSrcOptimal);
		if (m_PresentPass.ExecuteFunction != nullptr) 
		{
			cmd->BeginRendering({
				Arc::ColorAttachment{
				.ImageView = frameData.PresentImageView,
				.ImageLayout = Arc::ImageLayout::AttachmentOptimal,
				.LoadOp = m_PresentPass.LoadOp,
				.StoreOp = Arc::AttachmentStoreOp::Store,
				.ClearColor = m_PresentPass.ClearColor}
				}, { }, extent);
			m_PresentPass.ExecuteFunction(frameData.CommandBuffer, frameData.FrameIndex);
			cmd->EndRendering();
		}
		cmd->TransitionImage(frameData.PresentImage, Arc::ImageLayout::TransferSrcOptimal, Arc::ImageLayout::PresentSrc);
	}
}