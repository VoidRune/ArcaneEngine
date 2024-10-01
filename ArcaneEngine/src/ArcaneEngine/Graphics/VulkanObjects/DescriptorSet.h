#pragma once
#include "ArcaneEngine/Graphics/VulkanCore/VulkanHandles.h"
#include "ArcaneEngine/Graphics/Common.h"
#include <vector>

namespace Arc
{
	struct Binding
	{
		DescriptorType Type;
		ShaderStage ShaderStage;
	};

	struct DescriptorSetDesc
	{
		std::vector<Binding> Bindings;
	};

	class DescriptorSet
	{
	public:
		DescriptorSetHandle GetHandle() { return m_DescriptorSet; }
		// TODO check if binding index is valid
		DescriptorType GetBindingType(uint32_t binding) { return m_BindingTypes[binding]; }

	private:
		DescriptorSetHandle m_DescriptorSet;
		std::vector<DescriptorType> m_BindingTypes;

		friend class ResourceCache;
	};

	class DescriptorSetArray
	{
	public:
		DescriptorSetHandle GetHandle(uint32_t frameIndex) { return m_DescriptorSets[frameIndex]; }
		std::vector<DescriptorSetHandle> GetHandles() { return m_DescriptorSets; }
		// TODO check if binding index is valid
		DescriptorType GetBindingType(uint32_t binding) { return m_BindingTypes[binding]; }

	private:
		std::vector<DescriptorSetHandle> m_DescriptorSets;
		std::vector<DescriptorType> m_BindingTypes;

		friend class ResourceCache;
	};
}