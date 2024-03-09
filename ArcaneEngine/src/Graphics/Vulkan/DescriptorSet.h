#pragma once
#include "VkLocal.h"
#include "Common.h"

namespace Arc
{
	class DescriptorSetLayoutDesc
	{
	public:
		DescriptorSetLayoutDesc& AddBinding(uint32_t count, DescriptorType type, ShaderStage shaderStage);
		DescriptorSetLayoutDesc& AddFlag(DescriptorFlags flag);

		std::vector<VkDescriptorSetLayoutBinding> LayoutBindings;
		uint32_t Flags{};
	};

	class DescriptorSet
	{
	public:
		VkDescriptorSet& GetHandle(uint32_t inFlightIndex = 0) { return m_DescriptorSets[inFlightIndex & m_InFlight]; }
		VkDescriptorType GetBindingType(uint32_t binding) { return m_BindingTypes[binding]; }
	private:
		std::vector<VkDescriptorSet> m_DescriptorSets;
		std::vector<VkDescriptorType> m_BindingTypes;
		uint32_t m_InFlight = 0;

		friend class ResourceCache;
	};

	class DescriptorSetArray
	{
	public:
		VkDescriptorSet& GetHandle(uint32_t inFlightIndex) { return m_DescriptorSets[inFlightIndex]; }
	private:
		std::vector<VkDescriptorSet> m_DescriptorSets;

		friend class ResourceCache;
	};

	class DescriptorWriteDesc
	{
	public:
		DescriptorWriteDesc& AddBufferWrite(uint32_t binding, VkBuffer buffer, uint32_t offset, uint32_t range, uint32_t arrayElement = 0);
		DescriptorWriteDesc& AddImageWrite(uint32_t binding, VkSampler sampler, VkImageView imageView, ImageLayout imageLayout, uint32_t arrayElement = 0);

		std::list<VkDescriptorBufferInfo> BufferInfos;
		std::list<VkDescriptorImageInfo> ImageInfos;
		std::vector<VkWriteDescriptorSet> WriteInfo;
	};

	class DescriptorArrayWriteDesc
	{
	public:
		DescriptorArrayWriteDesc& AddBufferWrite(uint32_t binding, DescriptorType type, std::vector<VkBuffer> buffer, uint32_t offset, uint32_t range);
		//InFlightDescriptorWriteDesc& AddImageWrite(uint32_t binding, DescriptorType type, VkSampler sampler, VkImageView imageView, ImageLayout imageLayout);

		std::list<VkDescriptorBufferInfo> BufferInfos;
		//std::list<VkDescriptorImageInfo> imageInfos;
		std::vector<VkWriteDescriptorSet> WriteInfo;
	};
}