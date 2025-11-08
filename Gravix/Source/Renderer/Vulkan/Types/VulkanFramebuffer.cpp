#include "pch.h"
#include "VulkanFramebuffer.h"

#include "Renderer/Vulkan/Utils/VulkanInitializers.h"
#include "Renderer/Vulkan/Utils/VulkanUtils.h"

#include "Core/Application.h"

#include <backends/imgui_impl_vulkan.h>

namespace Gravix 
{


	static VkFormat ConvertTextureFormatToVkFormat(FramebufferTextureFormat format) 
	{
		switch (format)
		{
		case FramebufferTextureFormat::RGBA8:
			return VK_FORMAT_R8G8B8A8_UNORM;
		case FramebufferTextureFormat::RGBA16F:
			return VK_FORMAT_R16G16B16A16_SFLOAT;
		case FramebufferTextureFormat::RGBA32F:
			return VK_FORMAT_R32G32B32A32_SFLOAT;
		case FramebufferTextureFormat::RGBA32UI:
			return VK_FORMAT_R32G32B32A32_UINT;
		case FramebufferTextureFormat::Depth24SStencil8:
			return VK_FORMAT_D24_UNORM_S8_UINT;
		case FramebufferTextureFormat::Depth32FStencil8:
			return VK_FORMAT_D32_SFLOAT_S8_UINT;
		case FramebufferTextureFormat::Depth32:
			return VK_FORMAT_D32_SFLOAT;
		case FramebufferTextureFormat::RedInt:
			return VK_FORMAT_R32_SINT;
		case FramebufferTextureFormat::RedFloat:
			return VK_FORMAT_R32_SFLOAT;
		default:
			return VK_FORMAT_UNDEFINED;
		}
	}

	VulkanFramebuffer::VulkanFramebuffer(Device* device, const FramebufferSpecification& spec)
		: m_Device(static_cast<VulkanDevice*>(device)), m_UseSamples(spec.Multisampled)
	{
		Init(spec);
	}

	VulkanFramebuffer::~VulkanFramebuffer()
	{
		for (auto& attachment : m_Attachments)
		{
			vkDestroySampler(m_Device->GetDevice(), attachment.Sampler, nullptr);
			m_Device->DestroyImage(attachment.Image);
		}
	}

	void VulkanFramebuffer::StartFramebuffer(VkCommandBuffer cmd)
	{
		TransitionToLayout(cmd, VK_IMAGE_LAYOUT_GENERAL);
		
		m_ColorAttachments.clear();  // Clear and rebuild
		for (uint32_t i = 0; i < m_Attachments.size(); ++i)
		{
			if (i == m_DepthAttachmentIndex)
				continue;  // Skip depth attachment

			VkClearValue* pClearValue = nullptr;
			VkClearValue clearValue{};

			if (m_ClearColors.contains(i))
			{
				glm::vec4 color = m_ClearColors[i];
				clearValue.color = { {color.r, color.g, color.b, color.a} };
				pClearValue = &clearValue;
			}

			m_ColorAttachments.push_back(VulkanInitializers::AttachmentInfo(
				m_Attachments[i].Image.ImageView,
				pClearValue,
				VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
			));
		}
	}

	void VulkanFramebuffer::Resize(uint32_t width, uint32_t height)
	{
		if(width == m_Width && height == m_Height)
			return;

		m_Width = width;
		m_Height = height;

		for (size_t i = 0; i < m_Attachments.size(); i++)
		{
			CreateImage(i, width, height);
		}
	}

	void VulkanFramebuffer::SetClearColor(uint32_t index, const glm::vec4 clearColor)
	{
		m_ClearColors[index] = clearColor;
	}

	int VulkanFramebuffer::ReadPixel(uint32_t attachmentIndex, int mouseX, int mouseY)
	{
		if (attachmentIndex >= m_Attachments.size())
			return -1;

		AttachmentData& attachment = m_Attachments[attachmentIndex];

		int flippedY = m_Height - mouseY - 1;

		uint32_t pixelSize = 4;
		switch (attachment.Format)
		{
		case VK_FORMAT_R8_UINT: pixelSize = 1; break;
		case VK_FORMAT_R32_UINT: pixelSize = 4; break;
		case VK_FORMAT_R32_SINT: pixelSize = 4; break;
		case VK_FORMAT_R32G32B32A32_UINT: pixelSize = 16; break;
		case VK_FORMAT_R8G8B8A8_UNORM: pixelSize = 4; break;
		default: break;
		}

		// Create a small staging buffer using VMA (CPU-visible)
		AllocatedBuffer stagingBuffer = m_Device->CreateBuffer(
			pixelSize,
			VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			VMA_MEMORY_USAGE_CPU_ONLY
		);

		// Copy one pixel from the attachment image into the staging buffer
		m_Device->ImmediateSubmit([&, this](VkCommandBuffer cmd)
			{
				TransitionToLayout(cmd, attachmentIndex, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

				VkBufferImageCopy region{};
				region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				region.imageSubresource.mipLevel = 0;
				region.imageSubresource.baseArrayLayer = 0;
				region.imageSubresource.layerCount = 1;
				region.imageOffset = { mouseX, flippedY, 0 };
				region.imageExtent = { 1, 1, 1 };

				vkCmdCopyImageToBuffer(
					cmd,
					attachment.Image.Image,
					VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
					stagingBuffer.Buffer,
					1,
					&region
				);

				// Restore layout to color attachment
				TransitionToLayout(cmd, attachmentIndex, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
			});

		// Map and read pixel
		void* data;
		vmaMapMemory(m_Device->GetAllocator(), stagingBuffer.Allocation, &data);

		int result = -1;
		switch (attachment.Format)
		{
		case VK_FORMAT_R8_UINT:
			result = *(uint8_t*)data;
			break;
		case VK_FORMAT_R32_UINT:
			result = *(uint32_t*)data;
			break;
		case VK_FORMAT_R32_SINT:
			result = *(int32_t*)data;
			break;
		case VK_FORMAT_R32_SFLOAT:
			result = static_cast<int>((*(float*)data));
			break;
		case VK_FORMAT_R32G32B32A32_UINT:
			result = ((uint32_t*)data)[0];
			break;
		case VK_FORMAT_R8G8B8A8_UNORM:
		{
			uint8_t* px = (uint8_t*)data;
			result = px[0]; // red channel (adjust if you store ID elsewhere)
			break;
		}
		default:
			result = *(int*)data;
			break;
		}

		vmaUnmapMemory(m_Device->GetAllocator(), stagingBuffer.Allocation);
		m_Device->DestroyBuffer(stagingBuffer);

		return result;
	}


	void* VulkanFramebuffer::GetColorAttachmentID(uint32_t index)
	{
		AttachmentData& attachment = m_Attachments[index];

		if (m_DescriptorSets[index] != VK_NULL_HANDLE) 
		{
			ImGui_ImplVulkan_RemoveTexture(m_DescriptorSets[index]);
			m_DescriptorSets[index] = VK_NULL_HANDLE;
		}

		if (m_DescriptorSets[index] == VK_NULL_HANDLE)
		{
			m_DescriptorSets[index] = ImGui_ImplVulkan_AddTexture(
				attachment.Sampler,
				attachment.Image.ImageView,
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
			);
		}

		return (void*)m_DescriptorSets[index];
	}

	void VulkanFramebuffer::DestroyImGuiDescriptors()
	{
		for (uint32_t i = 0; i < m_DescriptorSets.size(); i++) 
		{
			VkDescriptorSet set = m_DescriptorSets[i];
			if(set == VK_NULL_HANDLE) continue;

			ImGui_ImplVulkan_RemoveTexture(set);
			m_DescriptorSets[i] = VK_NULL_HANDLE;
		}
	}

	void VulkanFramebuffer::TransitionToLayout(VkCommandBuffer cmd, VkImageLayout newLayout)
	{
		for(uint32_t i = 0; i < m_Attachments.size(); i++)
		{
			AttachmentData& attachment = m_Attachments[i];
			VulkanUtils::TransitionImage(cmd, attachment.Image.Image, attachment.Format, attachment.Layout, newLayout);

			attachment.Layout = newLayout;
		}
	}

	void VulkanFramebuffer::TransitionToLayout(VkCommandBuffer cmd, uint32_t index, VkImageLayout newLayout)
	{
		AttachmentData& attachment = m_Attachments[index];

		VulkanUtils::TransitionImage(cmd, attachment.Image.Image, attachment.Format, attachment.Layout, newLayout);
		attachment.Layout = newLayout;
	}

	void VulkanFramebuffer::TransitionDepthToShaderRead(VkCommandBuffer cmd)
	{
		if (m_DepthAttachmentIndex == -1) return;
		
		TransitionToLayout(cmd, m_DepthAttachmentIndex, VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_OPTIMAL);
	}

	void VulkanFramebuffer::TransitionToBeginRendering(VkCommandBuffer cmd)
	{
		for(uint32_t i = 0; i < m_Attachments.size(); i++)
		{
			AttachmentData& attachment = m_Attachments[i];
			VkImageLayout targetLayout = (i == m_DepthAttachmentIndex) ? VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL : VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
			if (attachment.Layout != targetLayout)
			{
				VulkanUtils::TransitionImage(cmd, attachment.Image.Image, attachment.Format, attachment.Layout, targetLayout);
				attachment.Layout = targetLayout;
			}
		}
	}

	std::vector<VkFormat> VulkanFramebuffer::GetColorAttachmentFormats() const
	{
		std::vector<VkFormat> formats;
		for (uint32_t i = 0; i < m_Attachments.size(); i++)
		{
			if (i == m_DepthAttachmentIndex)
				continue;

			formats.push_back(m_Attachments[i].Format);
		}

		return formats;
	}

	void VulkanFramebuffer::Init(const FramebufferSpecification& spec)
	{
		if (m_Width == 0 || m_Height == 0)
		{
			m_Width = Application::Get().GetWindow().GetWidth();
			m_Height = Application::Get().GetWindow().GetHeight();
		}

		VkExtent3D drawImageExtent = 
		{
			m_Width,
			m_Height,
			1
		};

		m_DescriptorSets.resize(spec.Attachments.size());

		for (uint32_t i = 0; i < spec.Attachments.size(); i++) 
		{
			FramebufferTextureFormat attachment = spec.Attachments[i];
			if (attachment == FramebufferTextureFormat::None)
				continue;

			VkFormat format = ConvertTextureFormatToVkFormat(attachment);
			VkImageUsageFlags attachmentUsage;
			bool isDepthAttachment = VulkanUtils::IsDepthFormat(format);

			if (isDepthAttachment)
				attachmentUsage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
			else 
				attachmentUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT;
			
			AllocatedImage image = m_Device->CreateImage(drawImageExtent, format, VK_IMAGE_USAGE_TRANSFER_DST_BIT
				| VK_IMAGE_USAGE_TRANSFER_SRC_BIT | attachmentUsage, spec.Multisampled);

			if (isDepthAttachment) 
			{
				m_DepthAttachmentIndex = i;
				VkClearValue depthClear{};
				depthClear.depthStencil.depth = 1.0f;  // Far plane for LESS compare
				depthClear.depthStencil.stencil = 0;
				m_DepthAttachment = VulkanInitializers::AttachmentInfo(image.ImageView, &depthClear, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);
			}
			else 
			{
				m_ColorAttachments.push_back(VulkanInitializers::AttachmentInfo(image.ImageView, nullptr));
			}

			VkSamplerCreateInfo samplerInfo{};
			samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
			samplerInfo.magFilter = VK_FILTER_LINEAR;
			samplerInfo.minFilter = VK_FILTER_LINEAR;
			samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
			samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
			samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
			samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
			samplerInfo.anisotropyEnable = VK_FALSE;
			samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
			samplerInfo.unnormalizedCoordinates = VK_FALSE;
			samplerInfo.compareEnable = VK_FALSE;
			samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
			samplerInfo.mipLodBias = 0.0f;
			samplerInfo.minLod = 0.0f;
			samplerInfo.maxLod = 0.0f;

			VkSampler sampler;
			if (vkCreateSampler(m_Device->GetDevice(), &samplerInfo, nullptr, &sampler) != VK_SUCCESS)
			{
				GX_CORE_CRITICAL("Failed to create sampler for framebuffer image.");
			}

			m_Attachments.push_back({ image, format, sampler, VK_IMAGE_LAYOUT_UNDEFINED });
		}
	}

	void VulkanFramebuffer::CreateImage(uint32_t index, uint32_t width, uint32_t height)
	{
		if (index >= m_Attachments.size()) 
			return;

		AttachmentData oldAttachment = m_Attachments[index];

		VkImageUsageFlags attachmentUsage;
		bool isDepthAttachment = VulkanUtils::IsDepthFormat(oldAttachment.Format);

		if (isDepthAttachment)
			attachmentUsage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
		else
			attachmentUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT;
			

		VkExtent3D drawImageExtent = {
			width,
			height,
			1
		};

		AllocatedImage image = m_Device->CreateImage(drawImageExtent, oldAttachment.Format, VK_IMAGE_USAGE_TRANSFER_DST_BIT
			| VK_IMAGE_USAGE_TRANSFER_SRC_BIT | attachmentUsage, m_UseSamples);
		
		if (isDepthAttachment)
		{
			VkClearValue depthClear{};
			depthClear.depthStencil.depth = 1.0f;  // Far plane for LESS compare
			depthClear.depthStencil.stencil = 0;
			m_DepthAttachment = VulkanInitializers::AttachmentInfo(image.ImageView, &depthClear, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);
		}
		else
		{
			if (m_ClearColors.contains(index)) 
			{
				glm::vec4 color = m_ClearColors[index];
				VkClearValue clearValue{};
				clearValue.color = { { color.r, color.g, color.b, color.a } };

				m_ColorAttachments[index] = VulkanInitializers::AttachmentInfo(image.ImageView, &clearValue);
			} else m_ColorAttachments[index] = (VulkanInitializers::AttachmentInfo(image.ImageView, nullptr));
		}

		VkSamplerCreateInfo samplerInfo{};
		samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		samplerInfo.magFilter = VK_FILTER_LINEAR;
		samplerInfo.minFilter = VK_FILTER_LINEAR;
		samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		samplerInfo.anisotropyEnable = VK_FALSE;
		samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
		samplerInfo.unnormalizedCoordinates = VK_FALSE;
		samplerInfo.compareEnable = VK_FALSE;
		samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
		samplerInfo.mipLodBias = 0.0f;
		samplerInfo.minLod = 0.0f;
		samplerInfo.maxLod = 0.0f;

		VkSampler sampler;
		if (vkCreateSampler(m_Device->GetDevice(), &samplerInfo, nullptr, &sampler) != VK_SUCCESS)
		{
			GX_CORE_CRITICAL("Failed to create sampler for framebuffer image.");
		}
		m_Attachments[index] = { image, oldAttachment.Format, sampler, VK_IMAGE_LAYOUT_UNDEFINED };

		m_Device->DestroyImage(oldAttachment.Image);
		vkDestroySampler(m_Device->GetDevice(), oldAttachment.Sampler, nullptr);

		m_DescriptorSets[index] = VK_NULL_HANDLE;
	}

}