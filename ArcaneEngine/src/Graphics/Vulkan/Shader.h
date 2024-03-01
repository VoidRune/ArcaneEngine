#pragma once
#include "VkLocal.h"

namespace Arc
{
    class ShaderDesc
    {
    public:

        std::vector<uint32_t> SpirV;
        std::string EntryPoint{ "main" };
        std::string FilePath;

        ShaderDesc& SetSpirv(const std::vector<uint32_t>& spirv);
        ShaderDesc& SetFilePath(const std::string& filePath);
        ShaderDesc& SetEntryPoint(const std::string& entryPoint);
    };

    class Shader
    {
    public:
        VkShaderModule GetShaderModule() { return m_ShaderModule; };
    private:
        VkShaderModule m_ShaderModule;
        VkShaderStageFlagBits m_ShaderStage;
        std::string m_EntryPoint;
        uint32_t m_StageOutputs;

        struct DescriptorLayoutBinding
        {
            VkDescriptorSetLayoutBinding binding;
            int32_t setIndex;
            friend class ResourceCache;
        };
        std::vector<DescriptorLayoutBinding> m_LayoutBindings;

        friend class ResourceCache;
        friend class Pipeline;
        friend class ComputePipeline;
    };
}