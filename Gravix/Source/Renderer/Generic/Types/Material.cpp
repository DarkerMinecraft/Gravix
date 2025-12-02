#include "pch.h"
#include "Material.h"

#include "Core/Application.h"
#include "Renderer/Vulkan/Types/VulkanMaterial.h"

namespace Gravix
{
	Ref<Material> Material::Create(AssetHandle shaderHandle, AssetHandle pipelineHandle)
	{
		Device* device = Application::Get().GetWindow().GetDevice();

		switch (device->GetType())
		{
		case DeviceType::None:    GX_VERIFY("DeviceType::None is currently not supported!"); return nullptr;
		case DeviceType::Vulkan:  return CreateRef<VulkanMaterial>(device, shaderHandle, pipelineHandle);
		}
		GX_VERIFY("Unknown RendererAPI!");
		return nullptr;
	}

	Ref<Material> Material::Create(Ref<Shader> shader, Ref<Pipeline> pipeline)
	{
		Device* device = Application::Get().GetWindow().GetDevice();

		switch (device->GetType())
		{
		case DeviceType::None:    GX_VERIFY("DeviceType::None is currently not supported!"); return nullptr;
		case DeviceType::Vulkan:  return CreateRef<VulkanMaterial>(device, shader, pipeline);
		}
		GX_VERIFY("Unknown RendererAPI!");
		return nullptr;
	}
}