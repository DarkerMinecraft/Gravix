#include "pch.h"
#include "Texture.h"

#include "Renderer/Vulkan/VulkanTexture.h"
#include "Core/Application.h"

namespace Gravix
{

	Ref<Texture2D> Texture2D::Create(const std::filesystem::path& path, const TextureSpecification& specification /*= TextureSpecification()*/)
	{
		Device* device = Application::Get().GetWindow().GetDevice();

		switch (device->GetType())
		{
		case DeviceType::None:    GX_STATIC_CORE_ASSERT("DeviceType::None is currently not supported!"); return nullptr;
		case DeviceType::Vulkan: return CreateRef<VulkanTexture2D>(device, path, specification);
		}
		GX_STATIC_CORE_ASSERT("Unknown RendererAPI!");
		return nullptr;
	}

	Ref<Texture2D> Texture2D::Create(void* data, uint32_t width /*= 1*/, uint32_t height /*= 1*/, const TextureSpecification& specification /*= TextureSpecification()*/)
	{
		Device* device = Application::Get().GetWindow().GetDevice();

		switch (device->GetType())
		{
		case DeviceType::None:    GX_STATIC_CORE_ASSERT("DeviceType::None is currently not supported!"); return nullptr;
		case DeviceType::Vulkan: return CreateRef<VulkanTexture2D>(device, data, width, height, specification);
		}
		GX_STATIC_CORE_ASSERT("Unknown RendererAPI!");
		return nullptr;
	}

}
