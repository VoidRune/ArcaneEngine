#pragma once
#include "ArcaneEngine/Graphics/VulkanCore/VulkanHandles.h"
#include "ArcaneEngine/Graphics/VulkanObjects/BottomLevelAS.h"
#include "ArcaneEngine/Graphics/Common.h"
#include <vector>

namespace Arc
{
	struct TopLevelASInstance
	{
		BottomLevelASHandle BottomLevelASHandle{};
		uint32_t InstanceCustomIndex{};
		float TransformMatrix[3][4];
	};

	struct TopLevelASDesc
	{

	};

	class Device;
	class TopLevelAS
	{
	public:
		void AddInstance(const TopLevelASInstance& instance);
		void ClearInstances();
		void Build();

		void Release();

		AccelerationStructureHandle GetHandle() { return m_Handle; }

	private:
		Device* m_Device;
		DeviceHandle m_LogicalDevice;
		AllocatorHandle m_Allocator;

		std::vector<TopLevelASInstance> m_Instances;

		AccelerationStructureHandle m_Handle{};
		BufferHandle m_Buffer{};
		AllocationHandle m_Allocation{};

		BufferHandle m_InstanceBuffer;
		AllocationHandle m_InstanceAllocation;

		friend class ResourceCache;
	};
}