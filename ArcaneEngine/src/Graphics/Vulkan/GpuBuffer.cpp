#include "GpuBuffer.h"

namespace Arc
{

    GpuBufferDesc& GpuBufferDesc::SetSize(uint32_t size)
    {
        this->Size = size;
        return *this;
    }

    GpuBufferDesc& GpuBufferDesc::AddBufferUsage(BufferUsage usageFlags)
    {
        this->UsageFlags |= static_cast<VkBufferUsageFlags>(usageFlags);
        return *this;
    }

    GpuBufferDesc& GpuBufferDesc::AddMemoryPropertyFlag(MemoryProperty memoryProperty)
    {
        this->MemoryVisibility |= static_cast<VkMemoryPropertyFlags>(memoryProperty);
        return *this;
    }
}