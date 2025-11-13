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
		BufferWrite(uint32_t binding, uint32_t arrayElement, GpuBuffer* buffer)
		{
			Binding = binding;
			ArrayElement = arrayElement;
			Buffer = buffer;
		}
		BufferWrite(uint32_t binding, GpuBuffer* buffer)
		{
			Binding = binding;
			Buffer = buffer;
		}

		uint32_t Binding = 0;
		uint32_t ArrayElement = 0;
		GpuBuffer* Buffer = nullptr;
	};

	struct BufferArrayWrite
	{
		BufferArrayWrite(uint32_t binding, uint32_t arrayElement, GpuBufferArray* buffer)
		{
			Binding = binding;
			ArrayElement = arrayElement;
			Buffer = buffer;
		}
		BufferArrayWrite(uint32_t binding, GpuBufferArray* buffer)
		{
			Binding = binding;
			Buffer = buffer;
		}

		uint32_t Binding = 0;
		uint32_t ArrayElement = 0;
		GpuBufferArray* Buffer = nullptr;
	};

	struct ImageWrite
	{
		ImageWrite(uint32_t binding, uint32_t arrayElement, GpuImage* image, ImageLayout imageLayout, Sampler* sampler)
		{
			Binding = binding;
			ArrayElement = arrayElement;
			Image = image;
			ImageLayout = imageLayout;
			Sampler = sampler;

		}
		ImageWrite(uint32_t binding, GpuImage* image, ImageLayout imageLayout, Sampler* sampler)
		{
			Binding = binding;
			Image = image;
			ImageLayout = imageLayout;
			Sampler = sampler;
		}

		uint32_t Binding = 0;
		uint32_t ArrayElement = 0;
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