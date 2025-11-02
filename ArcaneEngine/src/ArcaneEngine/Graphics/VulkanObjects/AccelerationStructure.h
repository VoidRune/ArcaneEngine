#pragma once
#include "ArcaneEngine/Graphics/VulkanCore/VulkanHandles.h"
#include "ArcaneEngine/Graphics/Common.h"
#include <vector>

namespace Arc
{
	struct BLAccelerationStructure 
	{
		BufferHandle VertexBuffer = nullptr;
		BufferHandle IndexBuffer = nullptr;
		uint32_t VertexStride = 0;
		uint32_t NumOfTriangles = 0;
		Format Format = Format::Undefined;
	};

	struct AccelerationStructureDesc
	{
		BLAccelerationStructure BLAccelerationStructure{};
	};

	class AccelerationStructure
	{
	public:
		AccelerationStructureHandle GetTopLevelHandle() { return m_TLAccelerationStructure.Handle; }

	private:
		struct AccelerationStructureInternal
		{
			AccelerationStructureHandle Handle;
			BufferHandle Buffer;
			AllocationHandle Allocation;
		};
		AccelerationStructureInternal m_TLAccelerationStructure;
		AccelerationStructureInternal m_BLAccelerationStructure;
		BufferHandle m_InstanceBuffer;
		AllocationHandle m_InstanceAllocation;

		friend class ResourceCache;
	};
}