#include "DescriptorSet.h"

namespace Arc
{
	DescriptorSetLayoutDesc& DescriptorSetLayoutDesc::AddBinding(uint32_t count, DescriptorType type, ShaderStage shaderStage)
	{
		VkDescriptorSetLayoutBinding binding;
		binding.binding = LayoutBindings.size();
		binding.descriptorCount = count;
		binding.descriptorType = static_cast<VkDescriptorType>(type);
		binding.stageFlags = static_cast<VkShaderStageFlags>(shaderStage);
		binding.pImmutableSamplers = nullptr;
		LayoutBindings.push_back(binding);

		return *this;
	}

	DescriptorSetLayoutDesc& DescriptorSetLayoutDesc::AddFlag(DescriptorFlags flag)
	{
		this->Flags |= static_cast<uint32_t>(flag);

		return *this;
	}

	DescriptorWriteDesc& DescriptorWriteDesc::AddBufferWrite(uint32_t binding, VkBuffer buffer, uint32_t offset, uint32_t range, uint32_t arrayElement)
	{
		VkDescriptorBufferInfo info;
		info.buffer = buffer;
		info.offset = offset;
		info.range = range;
		BufferInfos.push_back(info);

		VkWriteDescriptorSet write;
		write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		//write.dstSet = descriptorSet->GetHandle(i);
		write.dstBinding = binding;
		write.dstArrayElement = arrayElement;
		//write.descriptorType = static_cast<VkDescriptorType>(type);
		write.descriptorCount = 1;
		write.pBufferInfo = &BufferInfos.back();
		write.pImageInfo = nullptr;
		write.pTexelBufferView = nullptr;
		WriteInfo.push_back(write);

		return *this;
	}

	DescriptorWriteDesc& DescriptorWriteDesc::AddImageWrite(uint32_t binding, VkSampler sampler, VkImageView imageView, ImageLayout imageLayout, uint32_t arrayElement)
	{
		VkDescriptorImageInfo info;
		info.sampler = sampler;
		info.imageView = imageView;
		info.imageLayout = static_cast<VkImageLayout>(imageLayout);
		ImageInfos.push_back(info);

		VkWriteDescriptorSet write;
		write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		//write.dstSet = descriptorSet->GetHandle(i);
		write.dstBinding = binding;
		write.dstArrayElement = arrayElement;
		//write.descriptorType = static_cast<VkDescriptorType>(type);
		write.descriptorCount = 1;
		write.pBufferInfo = nullptr;
		write.pImageInfo = &ImageInfos.back();
		write.pTexelBufferView = nullptr;
		WriteInfo.push_back(write);

		return *this;
	}

	DescriptorArrayWriteDesc& DescriptorArrayWriteDesc::AddBufferWrite(uint32_t binding, DescriptorType type, std::vector<VkBuffer> buffer, uint32_t offset, uint32_t range)
	{
		for (size_t i = 0; i < buffer.size(); i++)
		{
			VkDescriptorBufferInfo info;
			info.buffer = buffer[i];
			info.offset = offset;
			info.range = range;
			BufferInfos.push_back(info);

			VkWriteDescriptorSet write;
			write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			//write.dstSet = descriptorSet->GetHandle(i);
			write.dstBinding = binding;
			write.dstArrayElement = 0;
			write.descriptorType = static_cast<VkDescriptorType>(type);
			write.descriptorCount = 1;
			write.pBufferInfo = &BufferInfos.back();
			write.pImageInfo = nullptr;
			write.pTexelBufferView = nullptr;
			WriteInfo.push_back(write);
		}

		return *this;
	}

}