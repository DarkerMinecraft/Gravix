#include "pch.h"
#include "Material.h"

#include "Core/Application.h"
#include "Renderer/Vulkan/VulkanMaterial.h"

namespace Gravix
{
	Ref<Material> Material::Create(const MaterialSpecification& spec)
	{
		Device* device = Application::Get().GetWindow().GetDevice();

		switch (device->GetType())
		{
		case DeviceType::None:    GX_STATIC_CORE_ASSERT("DeviceType::None is currently not supported!"); return nullptr;
		case DeviceType::Vulkan: return CreateRef<VulkanMaterial>(device, spec);
		}
		GX_STATIC_CORE_ASSERT("Unknown RendererAPI!");
		return nullptr;
	}

	Ref<Material> Material::Create(const std::string& debugName, const std::filesystem::path& shaderFilePath)
	{
		Device* device = Application::Get().GetWindow().GetDevice();

		switch (device->GetType())
		{
		case DeviceType::None:    GX_STATIC_CORE_ASSERT("DeviceType::None is currently not supported!"); return nullptr;
		case DeviceType::Vulkan:  return CreateRef<VulkanMaterial>(device, debugName, shaderFilePath);
		}
		GX_STATIC_CORE_ASSERT("Unknown RendererAPI!");
		return nullptr;
	}
}