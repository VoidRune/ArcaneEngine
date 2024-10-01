#include "ArcaneEngine/Graphics/ResourceCache.h"
#include "ArcaneEngine/Core/Log.h"
#include "ArcaneEngine/Graphics/VulkanCore/VulkanLocal.h"
#include <vulkan/vulkan_core.h>

namespace Arc
{
	void ResourceCache::CreateSampler(Sampler* sampler, const SamplerDesc& desc)
	{
        VkSamplerCreateInfo samplerInfo{};
        samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerInfo.magFilter = static_cast<VkFilter>(desc.MagFilter);
        samplerInfo.minFilter = static_cast<VkFilter>(desc.MinFilter);
        samplerInfo.addressModeU = static_cast<VkSamplerAddressMode>(desc.AddressMode);
        samplerInfo.addressModeV = samplerInfo.addressModeU;
        samplerInfo.addressModeW = samplerInfo.addressModeU;
        samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        samplerInfo.mipLodBias = 0.0f;
        samplerInfo.minLod = 0.0f;
        samplerInfo.maxLod = VK_LOD_CLAMP_NONE;
        samplerInfo.anisotropyEnable = VK_FALSE;
        samplerInfo.maxAnisotropy = 1.0f;
        samplerInfo.unnormalizedCoordinates = VK_FALSE;
        samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
        // Do not change compareEnable, it breaks the shadow mapping
        samplerInfo.compareEnable = VK_FALSE;
        samplerInfo.compareOp = VK_COMPARE_OP_NEVER;

        VkSampler samplerEx;
        VK_CHECK(vkCreateSampler((VkDevice)m_LogicalDevice, &samplerInfo, nullptr, &samplerEx));
        sampler->m_Sampler = samplerEx;

        m_Resources[sampler] = ResourceType::Sampler;

	}

    void ResourceCache::ReleaseResource(Sampler* sampler)
    {
        if (!m_Resources.contains(sampler))
        {
            ARC_LOG_FATAL("Cannot find Sampler resource to release!");
        }
        vkDestroySampler((VkDevice)m_LogicalDevice, (VkSampler)sampler->m_Sampler, nullptr);
        m_Resources.erase(sampler);
    }
}