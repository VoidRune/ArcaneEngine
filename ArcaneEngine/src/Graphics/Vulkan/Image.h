#pragma once
#include "VkLocal.h"
#include "Common.h"
#include "vk_mem_alloc.h"

namespace Arc
{
	struct GpuImageDesc
	{
	public:
		VkExtent2D Extent{};
		uint32_t Depth = 1;
		Format Format{};
		bool MipLevelsEnabled{};
		VkImageUsageFlags UsageFlags{};
		VkImageAspectFlags AspectFlags = VK_IMAGE_ASPECT_COLOR_BIT;

		GpuImageDesc& SetExtent(VkExtent2D extent);
		GpuImageDesc& SetDepth(uint32_t depth);
		GpuImageDesc& SetFormat(Arc::Format format);
		GpuImageDesc& SetEnableMipLevels(bool enable);
		GpuImageDesc& AddUsageFlag(ImageUsage usageFlag);
	};

	class GpuImage
	{
	public:
		VkImage GetHandle() { return m_Image; }
		VmaAllocation GetDeviceMemory() { return m_Allocation; }
		VkImageView GetImageView() { return m_ImageView; }
		uint32_t Width() { return m_Width; }
		uint32_t Height() { return m_Height; }
		uint32_t Depth() { return m_Depth; }
		uint32_t MipLevels() { return m_MipLevels; }

		struct Proxy
		{
			VkImage image;
			VkImageView imageView;
			VkFormat format;
		};
		Proxy GetProxy() { return { m_Image, m_ImageView, m_Format }; };
	private:
		uint32_t m_Width;
		uint32_t m_Height;
		uint32_t m_Depth;
		uint32_t m_MipLevels;
		VkImage m_Image;
		VmaAllocation m_Allocation;
		VkImageView m_ImageView;
		VkFormat m_Format;

		friend class ResourceCache;
	};

	struct SamplerDesc
	{
	public:
		Filter MinFilter{};
		Filter MagFilter{};
		SamplerAddressMode AddressMode{};

		SamplerDesc& SetMinFilter(Filter minFilter);
		SamplerDesc& SetMagFilter(Filter magFilter);
		SamplerDesc& SetAddressMode(SamplerAddressMode addressMode);
	};

	class Sampler
	{
	public:
		VkSampler GetHandle() { return m_Sampler; }
	private:
		VkSampler m_Sampler;

		friend class ResourceCache;
	};
}