#pragma once
#include "Vulkan/VkLocal.h"
#include "Vulkan/RenderPass.h"
#include "Vulkan/RenderPassCache.h"
#include "Vulkan/CommandBuffer.h"
#include "GpuProfiler.h"
#include <functional>

namespace Arc
{
	class RenderGraph
	{
	public:
		RenderGraph(VkDevice device, GpuProfiler* gpuProfiler);
		~RenderGraph();

		RenderPassProxy AddPass(const std::string& passName, const RenderPassDesc& desc);
		ComputePassProxy AddPass(const std::string& passName, const ComputePassDesc& desc);
		PresentPassProxy AddPass(const std::string& passName, const PresentPassDesc& desc);

		void SetRecordFunc(RenderPassProxy passProxy, std::function<void(CommandBuffer* cb, uint32_t frameIndex)> func);
		void SetRecordFunc(ComputePassProxy passProxy, std::function<void(CommandBuffer* cb, uint32_t frameIndex)> func);
		void SetRecordFunc(PresentPassProxy passProxy, std::function<void(CommandBuffer* cb, uint32_t frameIndex)> func);
		
		void BuildGraph();
		void Execute(FrameData frameData, VkExtent2D viewportExtent);

		VkRenderPass GetRenderPassHandle(RenderPassProxy& renderPassProxy) { return m_RenderPassTasks[renderPassProxy._renderGraphIndex].renderPass; }
		VkRenderPass GetRenderPassHandle(PresentPassProxy& presentPassProxy) { return m_PresentPassTasks[presentPassProxy.m_RenderGraphIndex].renderPass; }
	private:

		VkDevice m_Device;
		std::unique_ptr<RenderPassCache> m_RenderPassCache;
		GpuProfiler* m_GpuProfiler;

		struct Pass
		{
			enum class PassType
			{
				RenderPass,
				ComputePass,
				PresentPass,
			};
			std::string passName;
			PassType type;
			uint32_t index;
			std::vector<VkImageMemoryBarrier> imageTransitions;
		};
		struct RenderPassTask
		{
			VkExtent2D extent;
			VkRenderPass renderPass;
			VkFramebuffer framebuffer;
			std::vector<VkClearValue> clearValues;
			std::function<void(CommandBuffer* cb, uint32_t frameIndex)> function;
		};
		struct ComputePassTask
		{
			//ComputePassDesc desc;
			std::function<void(CommandBuffer* cb, uint32_t frameIndex)> function;
		};
		struct PresentPassTask
		{
			//ComputePassDesc desc;
			VkRenderPass renderPass;
			std::vector<uint32_t> framebufferKeys;
			std::vector<VkClearValue> clearValues;
			std::function<void(CommandBuffer* cb, uint32_t frameIndex)> function;
		};
		/* Resolved passes */
		std::vector<Pass> m_ResolvedPasses;
		std::vector<RenderPassTask> m_RenderPassTasks;
		std::vector<ComputePassTask> m_ComputePassTasks;
		std::vector<PresentPassTask> m_PresentPassTasks;

		/* Temporary passes (used during building) */
		std::vector<Pass> m_TempPasses;
		std::vector<RenderPassDesc> m_TempRenderPassDescs;
		std::vector<ComputePassDesc> m_TempComputePassDescs;
		std::vector<PresentPassDesc> m_TempPresentPassDescs;

	};
}