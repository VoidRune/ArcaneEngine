#pragma once
#include "VkLocal.h"
#include "Common.h"
#include "vk_mem_alloc.h"

namespace Arc
{
	struct ImageDesc
	{
	public:
		VkExtent2D Extent{};
		uint32_t Depth = 1;
		Format Format{};
		bool MipLevelsEnabled{};
		VkImageUsageFlags UsageFlags{};
		VkImageAspectFlags AspectFlags = VK_IMAGE_ASPECT_COLOR_BIT;

		ImageDesc& SetExtent(VkExtent2D extent);
		ImageDesc& SetDepth(uint32_t depth);
		ImageDesc& SetFormat(Arc::Format format);
		ImageDesc& SetEnableMipLevels(bool enable);
		ImageDesc& AddUsageFlag(ImageUsage usageFlag);
	};

	class Image
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