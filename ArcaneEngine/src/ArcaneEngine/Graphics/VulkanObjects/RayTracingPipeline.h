#pragma once
#include "ArcaneEngine/Graphics/VulkanCore/VulkanHandles.h"
#include "ArcaneEngine/Graphics/VulkanObjects/Shader.h"
#include "ArcaneEngine/Graphics/VulkanObjects/VertexAttributes.h"

namespace Arc
{
	struct RayTracingPipelineDesc
	{
		std::vector<Shader*> ShaderStages = {};
	};

	class RayTracingPipeline
	{
	public:
		PipelineHandle GetHandle() { return m_Pipeline; }
		PipelineLayoutHandle GetLayout() { return m_PipelineLayout; }

		struct StridedDeviceAddressRegion
		{
			uint64_t DeviceAddress;
			uint64_t Stride;
			uint64_t Size;
		};

		StridedDeviceAddressRegion GetRayGenShaderBindingTable() { return m_RayGenShaderBindingTable; }
		StridedDeviceAddressRegion GetRayMissShaderBindingTable() { return m_RayMissShaderBindingTable; }
		StridedDeviceAddressRegion GetRayClosestHitShaderBindingTable() { return m_RayClosestHitShaderBindingTable; }
	private:
		
		PipelineHandle m_Pipeline;
		PipelineLayoutHandle m_PipelineLayout;
		BufferHandle m_ShaderBindingTableBuffer;
		AllocationHandle m_ShaderBindingTableAllocation;

		StridedDeviceAddressRegion m_RayGenShaderBindingTable;
		StridedDeviceAddressRegion m_RayMissShaderBindingTable;
		StridedDeviceAddressRegion m_RayClosestHitShaderBindingTable;

		friend class ResourceCache;
	};
}