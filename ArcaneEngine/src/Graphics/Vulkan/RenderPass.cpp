#include "RenderPass.h"

namespace Arc
{
    
    RenderPassDesc& RenderPassDesc::SetInputImages(const std::vector<Image::Proxy>& imageInputs)
    {
        m_ImageInputs = imageInputs;
        return *this;
    }

    RenderPassDesc& RenderPassDesc::SetColorAttachments(const std::vector<Attachment>& attachments)
    {
        Attachment attachment;
        m_ColorAttachments = attachments;
        for (int i = 0; i < attachments.size(); i++)
        {
            m_ImageViews.push_back(attachments[i].proxy.imageView);

            VkClearValue clearValue = {};
            clearValue.color = { 1.0f, 0.0f, 1.0f, 1.0f };
            m_ClearValues.push_back(clearValue);
        }
        return *this;
    }

    RenderPassDesc& RenderPassDesc::SetDepthAttachment(Attachment attachment)
    {
        m_DepthAttachment = attachment;
        m_ImageViews.push_back(attachment.proxy.imageView);
        VkClearValue clearValue = {};
        clearValue.depthStencil = { 1.0f, 0 };
        m_ClearValues.push_back(clearValue);
        return *this;
    }

    RenderPassDesc& RenderPassDesc::SetExtent(VkExtent2D extent)
    {
        m_Extent = extent;
        return *this;
    }


    ComputePassDesc& ComputePassDesc::SetInputImages(const std::vector<Image::Proxy>& imageInputs)
    {
        m_ImageInputs = imageInputs;
        return *this;
    }

    ComputePassDesc& ComputePassDesc::SetOutputImages(const std::vector<Image::Proxy>& outputInputs)
    {
        m_ImageOutputs = outputInputs;
        return *this;
    }

    PresentPassDesc& PresentPassDesc::SetInputImages(const std::vector<Image::Proxy>& imageInputs)
    {
        m_ImageInputs = imageInputs;
        return *this;
    }

    PresentPassDesc& PresentPassDesc::SetFormat(VkFormat format)
    {
        m_Format = format;
        return *this;
    }

    PresentPassDesc& PresentPassDesc::SetImageViews(const std::vector<VkImageView>& imageViews)
    {
        m_ImageViews = imageViews;

        VkClearValue clearValue = {};
        clearValue.color = { 1.0f, 0.0f, 1.0f, 1.0f };
        m_ClearValues.push_back(clearValue);

        return *this;
    }

    PresentPassDesc& PresentPassDesc::SetExtent(VkExtent2D extent)
    {
        m_Extent = extent;
        return *this;
    }
}