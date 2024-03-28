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

	DescriptorArrayWriteDesc& DescriptorArrayWriteDesc::AddBufferWrite(uint32_t binding, std::vector<VkBuffer> buffer, uint32_t offset, uint32_t range)
	{
		std::vector<VkDescriptorBufferInfo> bufferInfos(buffer.size());
		for (size_t i = 0; i < bufferInfos.size(); i++)
		{
			bufferInfos[i].buffer = buffer[i];
			bufferInfos[i].offset = offset;
			bufferInfos[i].range = range;
		}
		BufferInfos[binding] = bufferInfos;

		return *this;
	}

	DescriptorArrayWriteDesc& DescriptorArrayWriteDesc::AddImageWrite(uint32_t binding, VkSampler sampler, VkImageView imageView, ImageLayout imageLayout)
	{
		std::vector<VkDescriptorImageInfo> imageInfos;

		VkDescriptorImageInfo info;
		info.sampler = sampler;
		info.imageView = imageView;
		info.imageLayout = static_cast<VkImageLayout>(imageLayout);
		imageInfos.push_back(info);

		ImageInfos[binding] = imageInfos;

		return *this;
	}

}