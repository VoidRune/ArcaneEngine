#pragma once
#include "ArcaneEngine/Graphics/VulkanCore/VulkanHandles.h"
#include "ArcaneEngine/Graphics/Common.h"

namespace Arc
{
	struct GpuImageDesc
	{
		uint32_t Extent[3] = {0, 0, 0};
		Format Format = Format::Undefined;
		ImageUsage UsageFlags = {};
		ImageAspect AspectFlags = ImageAspect::Color;
		uint32_t MipLevels = 1;
	};

	class GpuImage
	{
	public:
		ImageHandle GetHandle() { return m_Image; }
		ImageViewHandle GetImageView() { return m_ImageView; }
		AllocationHandle GetDeviceMemory() { return m_Allocation; }
		Format GetFormat() { return m_Format; }
		uint32_t* GetExtent() { return m_Extent; }

	private:
		ImageHandle m_Image;
		ImageViewHandle m_ImageView;
		AllocationHandle m_Allocation;
		Format m_Format;
		uint32_t m_Extent[3];

		friend class ResourceCache;
	};
}