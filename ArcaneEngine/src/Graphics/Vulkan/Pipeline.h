#pragma once
#include "VkLocal.h"
#include "Common.h"
#include "Shader.h"

namespace Arc
{
	class VertexAttributes
	{
	public:
		struct Attribute
		{
			Attribute(Format format, uint32_t offset) : format{ format }, offset{ offset } {};
			Format format;
			uint32_t offset;
		};
		VertexAttributes() {};
		VertexAttributes(const std::vector<Attribute>& inputAttributes, uint32_t vertexSize);
		~VertexAttributes() {};

		std::vector<VkVertexInputBindingDescription> InputBinding{};
		std::vector<VkVertexInputAttributeDescription> InputAttributes{};
	};
	
	struct PipelineDesc
	{
	public:
		std::vector<Shader*> ShaderStages;
		PrimitiveTopology Topology = PrimitiveTopology::TriangleList;
		CullMode CullMode = CullMode::Back;
		CompareOperation DepthCompare = CompareOperation::Less;
		bool DepthTestEnabled = true;
		bool DepthWriteEnabled = true;
		bool DepthBiasEnabled = false;
		VkRenderPass RenderPass;
		VertexAttributes VertexAttributes;

		PipelineDesc& AddShaderStage(Shader* shaderStage);
		PipelineDesc& SetPrimitiveTopology(PrimitiveTopology topology);
		PipelineDesc& SetCullMode(Arc::CullMode cullMode);
		PipelineDesc& SetDepthCompareOp(Arc::CompareOperation depthCompareOp);
		PipelineDesc& SetEnableDepthTest(bool depthTestEnabled);
		PipelineDesc& SetEnableDepthWrite(bool depthWriteEnabled);
		PipelineDesc& SetEnableDepthBias(bool depthBiasEnabled);
		PipelineDesc& SetRenderPass(VkRenderPass renderPass);
		PipelineDesc& SetVertexAttributes(Arc::VertexAttributes vertexAttributes);
	};

	class Pipeline
	{
	public:
		VkPipeline GetHandle() { return m_Pipeline; }
		VkPipelineLayout GetLayout() { return m_PipelineLayout; }

	private:

		VkPipeline m_Pipeline;
		VkPipelineLayout m_PipelineLayout;

		friend class ResourceCache;
	};

	struct ComputePipelineDesc
	{
	public:
		Shader* ShaderStage;

		ComputePipelineDesc& SetShaderStage(Shader* shaderStage);
	};

	class ComputePipeline
	{
	public:
		VkPipeline GetHandle() { return m_Pipeline; }
		VkPipelineLayout GetLayout() { return m_PipelineLayout; }

	private:

		VkPipeline m_Pipeline;
		VkPipelineLayout m_PipelineLayout;

		friend class ResourceCache;
	};
}