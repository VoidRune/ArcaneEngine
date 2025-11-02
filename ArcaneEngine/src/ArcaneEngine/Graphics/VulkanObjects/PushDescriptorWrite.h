#pragma once
#include "ArcaneEngine/Graphics/VulkanCore/VulkanHandles.h"
#include "GpuBuffer.h"
#include "GpuImage.h"
#include "Sampler.h"
#include "ArcaneEngine/Graphics/Common.h"
#include <vector>

namespace Arc
{
	struct PushBufferWrite
	{
		uint32_t Binding = 0;
		DescriptorType Type = {};
		BufferHandle Buffer = nullptr;
		uint32_t Size = 0;
	};

	struct PushImageWrite
	{
		uint32_t Binding = 0;
		DescriptorType Type = {};
		ImageViewHandle ImageView = nullptr;
		ImageLayout ImageLayout = ImageLayout::Undefined;
		SamplerHandle Sampler = nullptr;
	};

	struct PushAccelerationStructureWrite
	{
		uint32_t Binding = 0;
		DescriptorType Type = {};
		AccelerationStructureHandle AccelerationStructure = {};
	};

	class PushDescriptorWrite
	{
	public:
		PushDescriptorWrite& AddWrite(const PushBufferWrite& write)
		{
			m_BufferWrites.push_back(write);
			return *this;
		}
		PushDescriptorWrite& AddWrite(const PushImageWrite& write)
		{
			m_ImageWrites.push_back(write);
			return *this;
		}
		PushDescriptorWrite& AddWrite(const PushAccelerationStructureWrite& write)
		{
			m_AccelerationStructureWrites.push_back(write);
			return *this;
		}

	private:
		std::vector<PushBufferWrite> m_BufferWrites;
		std::vector<PushImageWrite> m_ImageWrites;
		std::vector<PushAccelerationStructureWrite> m_AccelerationStructureWrites;

		friend class Device;
		friend class CommandBuffer;
	};
}