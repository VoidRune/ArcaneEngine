#pragma once
#include "ArcaneEngine/Graphics/VulkanCore/VulkanHandles.h"
#include "ArcaneEngine/Graphics/VulkanObjects/Shader.h"

namespace Arc
{
	struct ComputePipelineDesc
	{
		Shader* Shader = nullptr;
		bool UsePushDescriptors = false;
	};

	class ComputePipeline
	{
	public:
		PipelineHandle GetHandle() { return m_Pipeline; }
		PipelineLayoutHandle GetLayout() { return m_PipelineLayout; }

	private:
		PipelineHandle m_Pipeline;
		PipelineLayoutHandle m_PipelineLayout;

		friend class ResourceCache;
	};
}