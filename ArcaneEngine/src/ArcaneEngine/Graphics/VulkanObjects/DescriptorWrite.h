#pragma once
#include "ArcaneEngine/Graphics/VulkanCore/VulkanHandles.h"
#include "GpuBuffer.h"
#include "GpuImage.h"
#include "Sampler.h"
#include "ArcaneEngine/Graphics/Common.h"
#include <vector>

namespace Arc
{
	struct BufferWrite
	{
		uint32_t Binding = 0;
		GpuBuffer* Buffer = nullptr;
	};

	struct BufferArrayWrite
	{
		uint32_t Binding = 0;
		GpuBufferArray* Buffer = nullptr;
	};

	struct ImageWrite
	{
		uint32_t Binding = 0;
		GpuImage* Image = nullptr;
		ImageLayout ImageLayout = ImageLayout::Undefined;
		Sampler* Sampler = nullptr;
	};

	class DescriptorWrite
	{
	public:
		DescriptorWrite& AddWrite(const BufferWrite& write)
		{
			m_BufferWrites.push_back(write);
			return *this;
		}
		DescriptorWrite& AddWrite(const BufferArrayWrite& write)
		{
			m_BufferArrayWrites.push_back(write);
			return *this;
		}
		DescriptorWrite& AddWrite(const ImageWrite& write)
		{
			m_ImageWrites.push_back(write);
			return *this;
		}

	private:
		std::vector<BufferWrite> m_BufferWrites;
		std::vector<BufferArrayWrite> m_BufferArrayWrites;
		std::vector<ImageWrite> m_ImageWrites;

		friend class Device;
		friend class CommandBuffer;
	};
}