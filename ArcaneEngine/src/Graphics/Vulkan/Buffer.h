#pragma once
#include "VkLocal.h"
#include "Common.h"
#include "vk_mem_alloc.h"

namespace Arc
{
	struct BufferDesc
	{
	public:
		uint32_t Size;
		VkBufferUsageFlags UsageFlags{};
		VkMemoryPropertyFlags MemoryVisibility{};

		BufferDesc& SetSize(uint32_t size);
		BufferDesc& AddBufferUsage(BufferUsage usageFlags);
		BufferDesc& AddMemoryPropertyFlag(MemoryProperty memoryProperty);
	};

	class Buffer
	{
	public:

		VkBuffer GetHandle() { return m_Buffer; };
		VmaAllocation GetMemory() { return m_Allocation; }
		uint32_t GetSize() { return m_Size; }

	private:

		VkBuffer m_Buffer;
		VmaAllocation m_Allocation;
		uint32_t m_Size;

		friend class Device;
		friend class ResourceCache;
	};

	class InFlightBuffer
	{
	public:

		std::vector<VkBuffer>& GetHandle() { return m_Buffer; };
		VmaAllocation GetMemory(uint32_t inFlightIndex) { return m_Allocation[inFlightIndex]; }
		uint32_t GetSize() { return m_Size; }

	private:

		std::vector<VkBuffer> m_Buffer;
		std::vector<VmaAllocation> m_Allocation;
		uint32_t m_Size;

		friend class Device;
		friend class ResourceCache;
	};
}