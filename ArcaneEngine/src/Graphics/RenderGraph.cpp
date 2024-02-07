#include "RenderGraph.h"

namespace Arc
{
	RenderGraph::RenderGraph(VkDevice device, GpuProfiler* gpuProfiler)
	{
		m_Device = device;
		m_RenderPassCache = std::make_unique<RenderPassCache>(device);
		m_GpuProfiler = gpuProfiler;
	}

	RenderGraph::~RenderGraph()
	{
		m_RenderPassCache.reset();
	}
	
	RenderPassProxy RenderGraph::AddPass(const std::string& passName, const RenderPassDesc& desc)
	{

		Pass p;
		p.passName = passName;
		p.type = Pass::PassType::RenderPass;
		p.index = m_TempRenderPassDescs.size();
		m_TempPasses.push_back(p);

		RenderPassProxy passProxy{};
		passProxy._renderGraphIndex = m_TempRenderPassDescs.size();

		m_TempRenderPassDescs.push_back(desc);

		return passProxy;
	}

	ComputePassProxy RenderGraph::AddPass(const std::string& passName, const ComputePassDesc& desc)
	{
		Pass p;
		p.passName = passName;
		p.type = Pass::PassType::ComputePass;
		p.index = m_TempComputePassDescs.size();
		m_TempPasses.push_back(p);

		ComputePassProxy passProxy{};
		passProxy.m_RenderGraphIndex = m_TempComputePassDescs.size();

		m_TempComputePassDescs.push_back(desc);

		return passProxy;
	}

	PresentPassProxy RenderGraph::AddPass(const std::string& passName, const PresentPassDesc& desc)
	{
		Pass p;
		p.passName = passName;
		p.type = Pass::PassType::PresentPass;
		p.index = m_TempPresentPassDescs.size();
		m_TempPasses.push_back(p);

		PresentPassProxy passProxy{};
		passProxy.m_RenderGraphIndex = m_TempPresentPassDescs.size();

		m_TempPresentPassDescs.push_back(desc);

		return passProxy;
	}

	void RenderGraph::BuildGraph()
	{
		m_RenderPassCache->Clear();
		m_ResolvedPasses.clear();
		m_RenderPassTasks.clear();
		m_ComputePassTasks.clear();
		m_PresentPassTasks.clear();

		std::vector<VkImage> usedAttachments;
		std::vector<Image::Proxy> inputImages;

		VkImageMemoryBarrier barrier{};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.pNext = nullptr;
		barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		barrier.oldLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL_KHR;
		barrier.newLayout = VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL_KHR;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.subresourceRange.baseMipLevel = 0;
		barrier.subresourceRange.levelCount = 1;
		barrier.subresourceRange.baseArrayLayer = 0;
		barrier.subresourceRange.layerCount = 1;


		// TODO: more sophisticated system 
		/* Constructs renderpasses and framebuffers						*/
		/* Checks if pass input is the same as any other pass output	*/
		/* all inputs and outputs of the SAME pass must be unique		*/
		for (auto& pass : m_TempPasses)
		{
			switch (pass.type)
			{
			case Pass::PassType::RenderPass:
			{
				auto& desc = m_TempRenderPassDescs[pass.index];

				RenderPassTask task;
				uint32_t renderPassKey =
					m_RenderPassCache->CreateRenderPass(desc,
						VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
				
				task.extent = desc.m_Extent;
				task.renderPass = m_RenderPassCache->GetRenderPass(renderPassKey);
				uint32_t framebufferKey = m_RenderPassCache->CreateFramebuffer(
					m_RenderPassCache->GetRenderPass(renderPassKey),
					desc.m_ImageViews, desc.m_Extent);
				task.framebuffer = m_RenderPassCache->GetFramebuffer(framebufferKey);
				task.clearValues = desc.m_ClearValues;

				/* add to inputs */
				for (auto& input : desc.m_ImageInputs)
					inputImages.push_back(input);
				/* add to used attachments */
				for (auto& attachment : desc.m_ColorAttachments)
					usedAttachments.push_back(attachment.proxy.image);
				/* add depth attachment */
				if (desc.m_DepthAttachment.proxy.format != VK_FORMAT_UNDEFINED)
					usedAttachments.push_back(desc.m_DepthAttachment.proxy.image);

				m_RenderPassTasks.push_back(task);
			}break;
			case Pass::PassType::ComputePass:
			{
				auto& desc = m_TempComputePassDescs[pass.index];

				ComputePassProxy passProxy{};
				passProxy.m_RenderGraphIndex = m_TempComputePassDescs.size();

				ComputePassTask task;
				task.function = nullptr;

				/* add to inputs */
				for (auto& input : desc.m_ImageInputs)
					inputImages.push_back(input);
				/* add to outputs */
				for (auto& output : desc.m_ImageOutputs)
					usedAttachments.push_back(output.image);
				m_ComputePassTasks.push_back(task);
			}break;
			case Pass::PassType::PresentPass:
			{
				auto& desc = m_TempPresentPassDescs[pass.index];

				PresentPassTask task;
				uint32_t renderPassKey =
					m_RenderPassCache->CreateRenderPass(RenderPassDesc()
						.SetColorAttachments({ { { nullptr, nullptr, desc.m_Format }, AttachmentLoadOp::Clear, AttachmentStoreOp::Store } }),
					VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

				task.renderPass = m_RenderPassCache->GetRenderPass(renderPassKey);
				task.clearValues = desc.m_ClearValues;

				for (auto& imageView : desc.m_ImageViews)
				{
					task.framebufferKeys.push_back(m_RenderPassCache->CreateFramebuffer(
						m_RenderPassCache->GetRenderPass(renderPassKey),
						{ imageView }, desc.m_Extent));
				}

				/* add to inputs */
				for (auto& input : desc.m_ImageInputs)
					inputImages.push_back(input);

				m_PresentPassTasks.push_back(task);
			}break;
			}


			/* Check if any input is same as attachment from previous render pass */
			for (auto& attachment : usedAttachments)
			{
				for (auto& input : inputImages)
				{
					if (attachment == input.image)
					{
						barrier.image = input.image;
						barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
						if (input.format == VK_FORMAT_D32_SFLOAT || input.format == VK_FORMAT_D16_UNORM ||
							input.format == VK_FORMAT_D16_UNORM_S8_UINT || input.format == VK_FORMAT_D24_UNORM_S8_UINT ||
							input.format == VK_FORMAT_D32_SFLOAT_S8_UINT)
						{
							barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
						}

						pass.imageTransitions.push_back(barrier);
					}
				}
			}
			inputImages.clear();

			m_ResolvedPasses.push_back(pass);
		}
		
		m_TempPasses.clear();
		m_TempRenderPassDescs.clear();
		m_TempComputePassDescs.clear();
		m_TempPresentPassDescs.clear();
	}

	void RenderGraph::SetRecordFunc(RenderPassProxy passProxy, std::function<void(CommandBuffer* cb, uint32_t frameIndex)> func)
	{
		m_RenderPassTasks[passProxy._renderGraphIndex].function = func;
	}

	void RenderGraph::SetRecordFunc(ComputePassProxy passProxy, std::function<void(CommandBuffer* cb, uint32_t frameIndex)> func)
	{
		m_ComputePassTasks[passProxy.m_RenderGraphIndex].function = func;
	}

	void RenderGraph::SetRecordFunc(PresentPassProxy passProxy, std::function<void(CommandBuffer* cb, uint32_t frameIndex)> func)
	{
		m_PresentPassTasks[passProxy.m_RenderGraphIndex].function = func;
	}

	void RenderGraph::Execute(FrameData frameData, VkExtent2D viewportExtent)
	{
		CommandBuffer* cb = frameData.CommandBuffer;

		m_GpuProfiler->BeginFrameProfiling(cb->GetHandle());

		for (auto& task : m_ResolvedPasses)
		{
			if (task.imageTransitions.size() > 0)
			{
				vkCmdPipelineBarrier(cb->GetHandle(),
					VK_PIPELINE_STAGE_VERTEX_SHADER_BIT,
					VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
					0,
					0, nullptr,
					0, nullptr,
					task.imageTransitions.size(), task.imageTransitions.data());
			}
			/*
			for (size_t i = 0; i < task.imageTransitions.size(); i++)
			{
				CommandBuffer::ImageTransitionInfo transitionInfo;
				transitionInfo.image = task.imageTransitions[i];
				transitionInfo.srcAccessMask = AccessFlags::ShaderWrite;
				transitionInfo.dstAccessMask = AccessFlags::ShaderRead;
				transitionInfo.oldLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL_KHR;
				transitionInfo.newLayout = VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL_KHR;
				transitionInfo.aspect = ImageAspect::Color;
				cb->ImageTransition(transitionInfo, PipelineStage::VertexShader, PipelineStage::FragmentShader);
			}*/

			switch (task.type)
			{
			case Pass::PassType::RenderPass:
			{
				auto& renderTask = m_RenderPassTasks[task.index];
				cb->SetViewport(renderTask.extent);
				cb->SetScissor(renderTask.extent);
				uint32_t gpuProfilingIndex = m_GpuProfiler->StartTask(task.passName);
				cb->BeginRenderPass(renderTask.renderPass, renderTask.framebuffer, renderTask.extent, renderTask.clearValues);
				renderTask.function(cb, frameData.FrameIndex);
				cb->EndRenderPass();
				m_GpuProfiler->EndTask(gpuProfilingIndex);

			}break;
			case Pass::PassType::ComputePass:
			{
				auto& computeTask = m_ComputePassTasks[task.index];

				uint32_t gpuProfilingIndex = m_GpuProfiler->StartTask(task.passName);
				computeTask.function(cb, frameData.FrameIndex);
				m_GpuProfiler->EndTask(gpuProfilingIndex);

			}break;
			case Pass::PassType::PresentPass:
			{
				auto& presentTask = m_PresentPassTasks[task.index];
				cb->SetViewport(viewportExtent);
				cb->SetScissor(viewportExtent);
				uint32_t gpuProfilingIndex = m_GpuProfiler->StartTask(task.passName);
				cb->BeginRenderPass(presentTask.renderPass,
					m_RenderPassCache->GetFramebuffer(presentTask.framebufferKeys[frameData.m_SwapchainImageIndex]),
					viewportExtent, presentTask.clearValues);
				presentTask.function(cb, frameData.FrameIndex);
				cb->EndRenderPass();
				m_GpuProfiler->EndTask(gpuProfilingIndex);

			}break;
			}
			
		}

		m_GpuProfiler->EndFrameProfiling();

	}
}