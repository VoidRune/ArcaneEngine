#include "Image.h"

namespace Arc
{
    GpuImageDesc& GpuImageDesc::SetExtent(VkExtent2D extent)
    {
        this->Extent = extent;
        return *this;
    }

    GpuImageDesc& GpuImageDesc::SetDepth(uint32_t depth)
    {
        this->Depth = depth;
        return *this;
    }

    GpuImageDesc& GpuImageDesc::SetFormat(Arc::Format format)
    {
        this->Format = format;
        return *this;
    }

    GpuImageDesc& GpuImageDesc::SetEnableMipLevels(bool enable)
    {
        this->MipLevelsEnabled = enable;
        return *this;
    }

    GpuImageDesc& GpuImageDesc::AddUsageFlag(ImageUsage usageFlag)
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