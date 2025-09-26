#include "pch.h"

#include "DescriptorWriter.h"
#include <vector>

namespace Gravix
{
	DescriptorWriter::DescriptorWriter(VkDescriptorSetLayout layout, VkDescriptorPool pool)
		: m_Layout(layout), m_Pool(pool)
	{
	}

	DescriptorWriter& DescriptorWriter::WriteBuffer(uint32_t binding, VkDescriptorBufferInfo* bufferInfo)
	{
		VkWriteDescriptorSet write{};
		write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		write.dstBinding = binding;
		write.dstArrayElement = 0;
		write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER; // Default, could be made configurable
		write.descriptorCount = 1;
		write.pBufferInfo = bufferInfo;
		write.pImageInfo = nullptr;
		write.pTexelBufferView = nullptr;

		m_Writes.push_back(write);
		return *this;
	}

	DescriptorWriter& DescriptorWriter::WriteImage(uint32_t binding, VkImageView imageView, VkSampler sampler, VkImageLayout imageLayout)
	{
		static thread_local VkDescriptorImageInfo imageInfo;
		imageInfo.sampler = sampler;
		imageInfo.imageView = imageView;
		imageInfo.imageLayout = imageLayout;

		VkWriteDescriptorSet write{};
		write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		write.dstBinding = binding;
		write.dstArrayElement = 0;
		write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		write.descriptorCount = 1;
		write.pBufferInfo = nullptr;
		write.pImageInfo = &imageInfo;
		write.pTexelBufferView = nullptr;

		m_Writes.push_back(write);
		return *this;
	}

	DescriptorWriter& DescriptorWriter::WriteImage(uint32_t binding, VkImageView imageView, VkImageLayout imageLayout)
	{
		static thread_local VkDescriptorImageInfo imageInfo;
		imageInfo.sampler = VK_NULL_HANDLE;  // Storage images don't use samplers
		imageInfo.imageView = imageView;
		imageInfo.imageLayout = imageLayout; // Typically VK_IMAGE_LAYOUT_GENERAL

		VkWriteDescriptorSet write{};
		write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		write.dstBinding = binding;
		write.dstArrayElement = 0;
		write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		write.descriptorCount = 1;
		write.pBufferInfo = nullptr;
		write.pImageInfo = &imageInfo;
		write.pTexelBufferView = nullptr;

		m_Writes.push_back(write);
		return *this;
	}

	void DescriptorWriter::Overwrite(VkDevice device, VkDescriptorSet set)
	{
		// Set the destination set for all writes
		for (auto& write : m_Writes)
		{
			write.dstSet = set;
		}

		// Execute all writes at once
		if (!m_Writes.empty())
		{
			vkUpdateDescriptorSets(device, static_cast<uint32_t>(m_Writes.size()), m_Writes.data(), 0, nullptr);
		}
	}
}
