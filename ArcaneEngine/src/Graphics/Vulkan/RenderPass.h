#pragma once
#include "VkLocal.h"
#include "Common.h"
#include "Image.h"

namespace Arc
{
    
    struct RenderPassDesc
    {
    public:
        struct Attachment
        {
            Attachment() {};
            Attachment(Image::Proxy proxy, AttachmentLoadOp loadOp, AttachmentStoreOp storeOp) : proxy{ proxy }, loadOp{ loadOp }, storeOp{storeOp} {};
            Image::Proxy proxy;
            AttachmentLoadOp loadOp;
            AttachmentStoreOp storeOp;
        };

        RenderPassDesc& SetInputImages(const std::vector<Image::Proxy>& imageInputs);
        RenderPassDesc& SetColorAttachments(const std::vector<Attachment>& attachments);
        RenderPassDesc& SetDepthAttachment(Attachment attachment);
        RenderPassDesc& SetExtent(VkExtent2D extent);

    private:
        std::vector<Image::Proxy> m_ImageInputs;
        std::vector<Attachment> m_ColorAttachments;
        std::vector<VkImageView> m_ImageViews;
        std::vector<VkClearValue> m_ClearValues;
        Attachment m_DepthAttachment = { { nullptr, nullptr, VK_FORMAT_UNDEFINED }, AttachmentLoadOp::Clear, AttachmentStoreOp::Store };
        VkExtent2D m_Extent;

        friend class RenderPassCache;
        friend class RenderGraph;
    };

    class RenderPassProxy
    {
    private:
        uint32_t _renderGraphIndex;

        friend class RenderGraph;
    };

    struct ComputePassDesc
    {
    public:
        ComputePassDesc& SetInputImages(const std::vector<Image::Proxy>& imageInputs);
        ComputePassDesc& SetOutputImages(const std::vector<Image::Proxy>& outputInputs);
    private:
        std::vector<Image::Proxy> m_ImageInputs;
        std::vector<Image::Proxy> m_ImageOutputs;

        friend class RenderGraph;
    };

    class ComputePassProxy
    {
    private:
        uint32_t m_RenderGraphIndex;

        friend class RenderGraph;
    };

    struct PresentPassDesc
    {
    public:
        PresentPassDesc& SetInputImages(const std::vector<Image::Proxy>& imageInputs);
        PresentPassDesc& SetFormat(VkFormat format);
        PresentPassDesc& SetImageViews(const std::vector<VkImageView>& imageViews);
        PresentPassDesc& SetExtent(VkExtent2D extent);

    private:
        std::vector<Image::Proxy> m_ImageInputs;
        VkFormat m_Format;
        std::vector<VkImageView> m_ImageViews;
        std::vector<VkClearValue> m_ClearValues;
        VkExtent2D m_Extent;

        friend class RenderGraph;
    };

    class PresentPassProxy
    {
    private:
        uint32_t m_RenderGraphIndex;

        friend class RenderGraph;
    };
}