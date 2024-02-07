#include "Image.h"

namespace Arc
{
    ImageDesc& ImageDesc::SetExtent(VkExtent2D extent)
    {
        this->Extent = extent;
        return *this;
    }

    ImageDesc& ImageDesc::SetDepth(uint32_t depth)
    {
        this->Depth = depth;
        return *this;
    }

    ImageDesc& ImageDesc::SetFormat(Arc::Format format)
    {
        this->Format = format;
        return *this;
    }

    ImageDesc& ImageDesc::SetEnableMipLevels(bool enable)
    {
        this->MipLevelsEnabled = enable;
        return *this;
    }

    ImageDesc& ImageDesc::AddUsageFlag(ImageUsage usageFlag)
    {
        VkImageUsageFlags vkFlag = static_cast<VkImageUsageFlags>(usageFlag);
        this->UsageFlags |= vkFlag;
        if (vkFlag & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT)
            this->AspectFlags = VK_IMAGE_ASPECT_DEPTH_BIT;
        return *this;
    }

    SamplerDesc& SamplerDesc::SetMinFilter(Filter minFilter)
    {
        this->MinFilter = minFilter;
        return *this;
    }

    SamplerDesc& SamplerDesc::SetMagFilter(Filter magFilter)
    {
        this->MagFilter = magFilter;
        return *this;
    }

    SamplerDesc& SamplerDesc::SetAddressMode(SamplerAddressMode addressMode)
    {
        this->AddressMode = addressMode;
        return *this;
    }
}