#include "pch.h"
#include "VulkanTexture.h"

#include "Renderer/Vulkan/Utils/VulkanUtils.h"

#include <backends/imgui_impl_vulkan.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

namespace Gravix
{

	VulkanTexture2D::VulkanTexture2D(Device* device, Buffer data, uint32_t width, uint32_t height, const TextureSpecification& specification)
		: m_Device(static_cast<VulkanDevice*>(device))
		, m_Specification(specification)
		, m_Width(width)
		, m_Height(height)
		, m_Channels(4) // Assume RGBA
	{
		CreateFromData(data, width, height, m_Channels);
	}

	VulkanTexture2D::~VulkanTexture2D()
	{
		Cleanup();
	}

	void* VulkanTexture2D::GetImGuiAttachment()
	{
		if (m_DescriptorSet == VK_NULL_HANDLE) 
			m_DescriptorSet = ImGui_ImplVulkan_AddTexture(m_Sampler, m_Image.ImageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		return (void*)m_DescriptorSet;
	}

	void VulkanTexture2D::DestroyImGuiDescriptor()
	{
		if(m_DescriptorSet != VK_NULL_HANDLE)
		{
			ImGui_ImplVulkan_RemoveTexture(m_DescriptorSet);
			m_DescriptorSet = VK_NULL_HANDLE;
		}
	}

	void VulkanTexture2D::CreateFromData(Buffer data, uint32_t width, uint32_t height, uint32_t channels)
	{
		if (!data)
		{
			return;
		}

		m_Width = width;
		m_Height = height;
		m_Channels = channels;

		// Calculate mip levels if mipmaps are enabled
		if (m_Specification.GenerateMipmaps)
		{
			m_MipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(m_Width, m_Height)))) + 1;
		}
		else
		{
			m_MipLevels = 1;
		}

		CreateVulkanResources(data, m_Width * m_Height * 4); // Assume RGBA = 4 bytes per pixel
		CreateSampler();
	}

	void VulkanTexture2D::CreateVulkanResources(Buffer data, uint32_t dataSize)
	{
		VkExtent3D imageExtent = { m_Width, m_Height, 1 };
		VkFormat imageFormat = VK_FORMAT_R8G8B8A8_UNORM; // Standard RGBA format

		// Create Vulkan image with transfer usage for uploading data
		VkImageUsageFlags usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
		if (m_Specification.GenerateMipmaps)
		{
			usage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT; // Needed for mipmap generation
		}

		// Use the device's CreateImage method with data - this handles mipmap generation internally
		m_Image = m_Device->CreateImage(static_cast<void*>(data.Data), imageExtent, imageFormat, usage, m_Specification.GenerateMipmaps);

		if (m_Image.Image == VK_NULL_HANDLE)
		{
			GX_CORE_ERROR("Failed to create Vulkan image for texture: {0}", m_Specification.DebugName);
			return;
		}

		m_Device->ImmediateSubmit([&](VkCommandBuffer cmd)
		{
			VulkanUtils::TransitionImage(cmd, m_Image.Image, m_Image.ImageFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		});

	}

	void VulkanTexture2D::CreateSampler()
	{
		VkSamplerCreateInfo samplerInfo = {};
		samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		samplerInfo.magFilter = ConvertFilter(m_Specification.MagFilter);
		samplerInfo.minFilter = ConvertFilter(m_Specification.MinFilter);
		samplerInfo.addressModeU = ConvertWrap(m_Specification.WrapS);
		samplerInfo.addressModeV = ConvertWrap(m_Specification.WrapT);
		samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;

		// Anisotropic filtering
		samplerInfo.anisotropyEnable = VK_TRUE;
		samplerInfo.maxAnisotropy = 16.0f; // TODO: Get from device capabilities

		samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
		samplerInfo.unnormalizedCoordinates = VK_FALSE;
		samplerInfo.compareEnable = VK_FALSE;
		samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;

		// Mipmap settings
		samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		samplerInfo.mipLodBias = 0.0f;
		samplerInfo.minLod = 0.0f;
		samplerInfo.maxLod = static_cast<float>(m_MipLevels);

		VkResult result = vkCreateSampler(m_Device->GetDevice(), &samplerInfo, nullptr, &m_Sampler);
		if (result != VK_SUCCESS)
		{
			GX_CORE_ERROR("Failed to create texture sampler for '{0}': {1}", m_Specification.DebugName, static_cast<int>(result));
			return;
		}
	}

	VkDescriptorImageInfo VulkanTexture2D::GetDescriptorInfo() const
	{
		VkDescriptorImageInfo imageInfo{};
		imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		imageInfo.imageView = m_Image.ImageView;
		imageInfo.sampler = m_Sampler;
		return imageInfo;
	}

	void VulkanTexture2D::Cleanup()
	{
		vkDeviceWaitIdle(m_Device->GetDevice());

		if (m_Sampler != VK_NULL_HANDLE)
		{
			vkDestroySampler(m_Device->GetDevice(), m_Sampler, nullptr);
			m_Sampler = VK_NULL_HANDLE;
		}

		if (m_Image.Image != VK_NULL_HANDLE)
		{
			m_Device->DestroyImage(m_Image);
			m_Image = {};
		}
	}

	void VulkanTexture2D::CreateMagentaTexture()
	{
		m_Width = 4;
		m_Height = 4;
		m_Channels = 4;
		
		uint32_t black = glm::packUnorm4x8(glm::vec4(0, 0, 0, 1));
		uint32_t magenta = glm::packUnorm4x8(glm::vec4(1, 0, 1, 1));
		std::array<uint32_t, 16 * 16> pixels; //for 16x16 checkerboard texture
		for (int x = 0; x < 16; x++) {
			for (int y = 0; y < 16; y++) {
				pixels[y * 16 + x] = ((x % 2) ^ (y % 2)) ? magenta : black;
			}
		}

		Buffer buf;
		buf.Data = reinterpret_cast<uint8_t*>(pixels.data());
		buf.Size = sizeof(uint32_t) * pixels.size();

		CreateFromData(buf, 16, 16, 4);
	}

	VkFilter VulkanTexture2D::ConvertFilter(TextureFilter filter) const
	{
		switch (filter)
		{
		case TextureFilter::Nearest: return VK_FILTER_NEAREST;
		case TextureFilter::Linear:  return VK_FILTER_LINEAR;
		default:                    return VK_FILTER_LINEAR;
		}
	}

	VkSamplerAddressMode VulkanTexture2D::ConvertWrap(TextureWrap wrap) const
	{
		switch (wrap)
		{
		case TextureWrap::Repeat:       return VK_SAMPLER_ADDRESS_MODE_REPEAT;
		case TextureWrap::ClampToEdge:  return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		case TextureWrap::ClampToBorder:return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
		default:                       return VK_SAMPLER_ADDRESS_MODE_REPEAT;
		}
	}
}