#pragma once
#include "ArcaneEngine/Graphics/VulkanCore/VulkanHandles.h"
#include "ArcaneEngine/Graphics/Common.h"
#include <array>

namespace Arc
{
	class ColorAttachment
	{
	public:

		ImageViewHandle ImageView = {};
		ImageLayout ImageLayout = {};
		AttachmentLoadOp LoadOp = AttachmentLoadOp::Clear;
		AttachmentStoreOp StoreOp = AttachmentStoreOp::Store;
		std::array<float, 4> ClearColor = {};
	};

	class DepthAttachment
	{
	public:

		ImageViewHandle ImageView = {};
		ImageLayout ImageLayout = {};
		AttachmentLoadOp LoadOp = AttachmentLoadOp::Clear;
		AttachmentStoreOp StoreOp = AttachmentStoreOp::Store;
		float ClearValue = {};
	};
}