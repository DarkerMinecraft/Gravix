#pragma once

#include <vulkan/vulkan.h>

namespace Gravix
{
	class DescriptorWriter
	{
	public:
		DescriptorWriter(VkDescriptorSetLayout layout, VkDescriptorPool pool);

		DescriptorWriter& WriteBuffer(uint32_t binding, VkDescriptorBufferInfo* bufferInfo);
		DescriptorWriter& WriteImage(uint32_t binding, VkImageView imageView, VkSampler sampler, VkImageLayout imageLayout);
		DescriptorWriter& WriteImage(uint32_t binding, uint32_t index, VkImageView imageView, VkSampler sampler, VkImageLayout imageLayout);
		DescriptorWriter& WriteImage(uint32_t binding, VkImageView imageView, VkImageLayout imageLayout);

		void Overwrite(VkDevice device, VkDescriptorSet set);

	private:
		VkDescriptorSetLayout m_Layout;
		VkDescriptorPool m_Pool;
		std::vector<VkWriteDescriptorSet> m_Writes;
		std::vector<VkDescriptorImageInfo> m_ImageInfos;  
		std::vector<VkDescriptorBufferInfo> m_BufferInfos; 
	};
}