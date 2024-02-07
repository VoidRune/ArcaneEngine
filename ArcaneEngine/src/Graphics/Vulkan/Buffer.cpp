#include "Buffer.h"

namespace Arc
{

    BufferDesc& BufferDesc::SetSize(uint32_t size)
    {
        this->Size = size;
        return *this;
    }

    BufferDesc& BufferDesc::AddBufferUsage(BufferUsage usageFlags)
    {
        this->UsageFlags |= static_cast<VkBufferUsageFlags>(usageFlags);
        return *this;
    }

    BufferDesc& BufferDesc::AddMemoryPropertyFlag(MemoryProperty memoryProperty)
    {
        this->MemoryVisibility |= static_cast<VkMemoryPropertyFlags>(memoryProperty);
        return *this;
    }
}