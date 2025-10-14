#include "pch.h"

#include "DescriptorWriter.h"
#include <vector>

namespace Gravix
{

	static const char* GetDescriptorTypeName(VkDescriptorType type)
	{
		switch (type)
		{
		case VK_DESCRIPTOR_TYPE_SAMPLER: return "SAMPLER";
		case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER: return "COMBINED_IMAGE_SAMPLER";
		case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE: return "SAMPLED_IMAGE";
		case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE: return "STORAGE_IMAGE";
		case VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER: return "UNIFORM_TEXEL_BUFFER";
		case VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER: return "STORAGE_TEXEL_BUFFER";
		case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER: return "UNIFORM_BUFFER";
		case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER: return "STORAGE_BUFFER";
		case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC: return "UNIFORM_BUFFER_DYNAMIC";
		case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC: return "STORAGE_BUFFER_DYNAMIC";
		case VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT: return "INPUT_ATTACHMENT";
		default: return "UNKNOWN";
		}
	}

	DescriptorWriter::DescriptorWriter(VkDescriptorSetLayout layout, VkDescriptorPool pool)
		: m_Layout(layout), m_Pool(pool)
	{
	}

	DescriptorWriter& DescriptorWriter::WriteBuffer(uint32_t binding, VkDescriptorBufferInfo* bufferInfo)
	{
		m_BufferInfos.push_back(*bufferInfo);  

		VkWriteDescriptorSet write{};
		write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		write.dstBinding = binding;
		write.dstArrayElement = 0;
		write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		write.descriptorCount = 1;
		write.pBufferInfo = &m_BufferInfos.back();  
		write.pImageInfo = nullptr;
		write.pTexelBufferView = nullptr;

		m_Writes.push_back(write);
		return *this;
	}

	DescriptorWriter& DescriptorWriter::WriteImage(uint32_t binding, VkImageView imageView, VkSampler sampler, VkImageLayout imageLayout)
	{
		VkDescriptorImageInfo imageInfo{};
		imageInfo.sampler = sampler;
		imageInfo.imageView = imageView;
		imageInfo.imageLayout = imageLayout;

		m_ImageInfos.push_back(imageInfo); 

		VkWriteDescriptorSet write{};
		write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		write.dstBinding = binding;
		write.dstArrayElement = 0;
		write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		write.descriptorCount = 1;
		write.pBufferInfo = nullptr;
		write.pImageInfo = &m_ImageInfos.back(); 
		write.pTexelBufferView = nullptr;

		m_Writes.push_back(write);
		return *this;
	}

	DescriptorWriter& DescriptorWriter::WriteImage(uint32_t binding, uint32_t index, VkImageView imageView, VkSampler sampler, VkImageLayout imageLayout)
	{
		VkDescriptorImageInfo imageInfo{};
		imageInfo.sampler = sampler;
		imageInfo.imageView = imageView;
		imageInfo.imageLayout = imageLayout;

		m_ImageInfos.push_back(imageInfo); 

		VkWriteDescriptorSet write{};
		write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		write.dstBinding = binding;
		write.dstArrayElement = index;  // Array element
		write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		write.descriptorCount = 1;
		write.pBufferInfo = nullptr;
		write.pImageInfo = &m_ImageInfos.back();  
		write.pTexelBufferView = nullptr;

		m_Writes.push_back(write);
		return *this;
	}

	DescriptorWriter& DescriptorWriter::WriteImage(uint32_t binding, VkImageView imageView, VkImageLayout imageLayout)
	{
		VkDescriptorImageInfo imageInfo{};
		imageInfo.sampler = VK_NULL_HANDLE;  // Storage images don't use samplers
		imageInfo.imageView = imageView;
		imageInfo.imageLayout = imageLayout;

		m_ImageInfos.push_back(imageInfo);  

		VkWriteDescriptorSet write{};
		write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		write.dstBinding = binding;
		write.dstArrayElement = 0;
		write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		write.descriptorCount = 1;
		write.pBufferInfo = nullptr;
		write.pImageInfo = &m_ImageInfos.back();  
		write.pTexelBufferView = nullptr;

		m_Writes.push_back(write);
		return *this;
	}

	void DescriptorWriter::Overwrite(VkDevice device, VkDescriptorSet set)
	{
		if (m_Writes.empty())
		{
			GX_CORE_WARN("DescriptorWriter::Overwrite called with no writes - nothing to update");
			return;
		}

		// Set the destination set for all writes
		for (auto& write : m_Writes)
		{
			write.dstSet = set;
		}

		for (size_t i = 0; i < m_Writes.size(); ++i)
		{
			const auto& write = m_Writes[i];
			const char* typeStr = GetDescriptorTypeName(write.descriptorType);

			// Validate write data
			if (write.descriptorType == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER ||
				write.descriptorType == VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE ||
				write.descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_IMAGE)
			{
				if (write.pImageInfo == nullptr)
				{
					GX_CORE_ERROR("  ERROR: pImageInfo is null for image descriptor!");
				}
			}
			else if (write.descriptorType == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER ||
				write.descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER)
			{
				if (write.pBufferInfo == nullptr)
				{
					GX_CORE_ERROR("  ERROR: pBufferInfo is null for buffer descriptor!");
				}
			}
		}

		// Perform the descriptor update
		vkUpdateDescriptorSets(device, static_cast<uint32_t>(m_Writes.size()),
			m_Writes.data(), 0, nullptr);

		// Clear for reuse - safe to clear after vkUpdateDescriptorSets completes
		m_Writes.clear();
		m_ImageInfos.clear();
		m_BufferInfos.clear();
	}

}
