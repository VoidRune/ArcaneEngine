#pragma once
#include "ArcaneEngine/Graphics/VulkanCore/VulkanHandles.h"
#include "ArcaneEngine/Graphics/VulkanObjects/Shader.h"
#include "ArcaneEngine/Graphics/VulkanObjects/VertexAttributes.h"

namespace Arc
{
	struct PipelineDesc
	{
		std::vector<Shader*> ShaderStages = {};
		PrimitiveTopology Topology = PrimitiveTopology::TriangleList;
		CullMode CullMode = CullMode::Back;
		bool DepthTest = true;
		bool DepthWrite = true;
		CompareOperation DepthCompareOp = CompareOperation::Less;
		VertexAttributes VertexAttributes = {};
		std::vector<Format> ColorAttachmentFormats = {};
		Format DepthAttachmentFormat = Format::Undefined;
		bool UsePushDescriptors = false;
	};

	class Pipeline
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