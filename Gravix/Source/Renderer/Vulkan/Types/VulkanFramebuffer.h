#pragma once

#include "Renderer/Generic/Types/Framebuffer.h"

#include "Renderer/Vulkan/Utils/VulkanTypes.h"
#include "Renderer/Vulkan/VulkanDevice.h"
#include "Renderer/Vulkan/VulkanRenderCaps.h"

namespace Gravix 
{

	struct AttachmentData 
	{
		AllocatedImage Image;
		VkFormat Format;
		VkSampler Sampler;

		VkImageLayout Layout;
	};

	class VulkanFramebuffer : public Framebuffer
	{	
	public:
		VulkanFramebuffer(Device* device, const FramebufferSpecification& spec);
		virtual ~VulkanFramebuffer();

		void StartFramebuffer(VkCommandBuffer cmd);

		virtual uint32_t GetWidth() const override { return m_Width; }
		virtual uint32_t GetHeight() const override { return m_Height; }
		
		virtual void Resize(uint32_t width, uint32_t height) override;

		virtual void* GetColorAttachmentID(uint32_t index) override;
		virtual void DestroyImGuiDescriptors() override;

		virtual void SetClearColor(uint32_t index, const glm::vec4 clearColor) override;
		virtual void SetClearColor(uint32_t index, const glm::ivec4 clearColor) override;
		
		virtual int ReadPixel(uint32_t attachmentIndex, int mouseX, int mouseY) override;

		void TransitionToLayout(VkCommandBuffer cmd, VkImageLayout newLayout);
		void TransitionToLayout(VkCommandBuffer cmd, uint32_t index, VkImageLayout newLayout);

		void TransitionDepthToShaderRead(VkCommandBuffer cmd);

		void TransitionToBeginRendering(VkCommandBuffer cmd);

		uint32_t GetDepthAttachmentIndex() const { return m_DepthAttachmentIndex; }

		std::vector<VkRenderingAttachmentInfo> GetColorAttachments() { return m_ColorAttachments; };
		VkRenderingAttachmentInfo* GetDepthAttachment() { return m_DepthAttachmentIndex == -1 ? nullptr : &m_DepthAttachment; }

		const std::vector<AttachmentData> GetAttachments() const { return m_Attachments; }

		AllocatedImage GetImage(uint32_t index) const { return m_Attachments[index].Image; }
		VkImageView GetAttachmentImageView(uint32_t index) const { return m_Attachments[index].Image.ImageView; }
		VkFormat GetImageFormat(uint32_t index) const { return m_Attachments[index].Format; }
		VkFormat GetColorFormat(uint32_t index) const { return m_Attachments[index].Format; }  // Alias
		VkFormat GetDepthFormat() const { return m_DepthAttachmentIndex != -1 ? m_Attachments[m_DepthAttachmentIndex].Format : VK_FORMAT_UNDEFINED; }
		VkSampler GetImageSampler(uint32_t index) const { return m_Attachments[index].Sampler; }

		std::vector<VkFormat> GetColorAttachmentFormats() const;

		bool IsUsingSamples() const { return m_UseSamples; }
		VkSampleCountFlagBits GetSampleCount() const { return m_UseSamples ? VulkanRenderCaps::GetSampleCount() : VK_SAMPLE_COUNT_1_BIT; }
	private:
		void Init(const FramebufferSpecification& spec);
		void CreateImage(uint32_t index, uint32_t width, uint32_t height);
	private:
		VulkanDevice* m_Device;

		std::vector<AttachmentData> m_Attachments;
		uint32_t m_Width = 0, m_Height = 0;

		std::vector<VkRenderingAttachmentInfo> m_ColorAttachments;
		VkRenderingAttachmentInfo m_DepthAttachment;

		uint32_t m_DepthAttachmentIndex = -1;
		bool m_UseSamples;

		std::vector<VkDescriptorSet> m_DescriptorSets;
		std::map<uint32_t, glm::vec4> m_ClearColors;
		std::map<uint32_t, glm::ivec4> m_ClearColorsInt;
	};

}