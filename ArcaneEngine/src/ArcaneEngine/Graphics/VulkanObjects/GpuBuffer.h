#pragma once
#include "ArcaneEngine/Graphics/VulkanCore/VulkanHandles.h"
#include "ArcaneEngine/Graphics/Common.h"
#include <vector>

namespace Arc
{
	struct GpuBufferDesc
	{
		uint32_t Size;
		BufferUsage UsageFlags;
		MemoryProperty MemoryProperty;
	};

	class GpuBuffer
	{
	public:
		BufferHandle GetHandle() { return m_Buffer; }
		uint32_t GetSize() { return m_Size; }

	private:
		BufferHandle m_Buffer;
		AllocationHandle m_Allocation;
		uint32_t m_Size;
		
		friend class ResourceCache;
	};

	class GpuBufferArray
	{
	public:
		BufferHandle GetHandle(uint32_t frameIndex) { return m_Buffers[frameIndex]; }
		uint32_t GetSize() { return m_Size; }

	private:
		std::vector<BufferHandle> m_Buffers;
		std::vector<AllocationHandle> m_Allocations;
		uint32_t m_Size;

		friend class ResourceCache;
	};
}