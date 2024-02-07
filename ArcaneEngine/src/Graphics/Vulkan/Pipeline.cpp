#include "Pipeline.h"

namespace Arc
{
    VertexAttributes::VertexAttributes(const std::vector<Attribute>& inputAttributes, uint32_t vertexSize)
    {
        for (int i = 0; i < inputAttributes.size(); i++)
        {
            VkVertexInputAttributeDescription attribute;
            attribute.binding = 0;
            attribute.location = i;
            attribute.format = static_cast<VkFormat>(inputAttributes[i].format);
            attribute.offset = inputAttributes[i].offset;
            InputAttributes.push_back(attribute);
        }
        if (vertexSize > 0)
        {
            VkVertexInputBindingDescription binding;
            binding.binding = 0;
            binding.stride = vertexSize;
            binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
            InputBinding.push_back(binding);
        }
    }

    PipelineDesc& PipelineDesc::AddShaderStage(Shader* shaderStage)
    {
        ShaderStages.push_back(shaderStage);
        return *this;
    }
    PipelineDesc& PipelineDesc::SetPrimitiveTopology(PrimitiveTopology topology)
    {
        this->Topology = topology;
        return *this;
    }
    PipelineDesc& PipelineDesc::SetCullMode(Arc::CullMode cullMode)
    {
        this->CullMode = cullMode;
        return *this;
    }
    PipelineDesc& PipelineDesc::SetDepthCompareOp(Arc::CompareOperation depthCompareOp)
    {
        this->DepthCompare = depthCompareOp;
        return *this;
    }
    PipelineDesc& PipelineDesc::SetEnableDepthTest(bool depthTestEnabled)
    {
        this->DepthTestEnabled = depthTestEnabled;
        return *this;
    }
    PipelineDesc& PipelineDesc::SetEnableDepthWrite(bool depthWriteEnabled)
    {
        this->DepthWriteEnabled = depthWriteEnabled;
        return *this;
    }
    PipelineDesc& PipelineDesc::SetEnableDepthBias(bool depthBiasEnabled)
    {
        this->DepthBiasEnabled = depthBiasEnabled;
        return *this;
    }
    PipelineDesc& PipelineDesc::SetRenderPass(VkRenderPass renderPass)
    {
        this->RenderPass = renderPass;
        return *this;
    }
    PipelineDesc& PipelineDesc::SetVertexAttributes(Arc::VertexAttributes vertexAttributes)
    {
        this->VertexAttributes = vertexAttributes;
        return *this;
    }


    ComputePipelineDesc& ComputePipelineDesc::SetShaderStage(Shader* shaderStage)
    {
        this->ShaderStage = shaderStage;
        return *this;
    }

}