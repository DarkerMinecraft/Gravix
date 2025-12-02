#include "pch.h"
#include "Shader.h"

#include "Core/Application.h"
#include "Renderer/Vulkan/Types/VulkanShader.h"

namespace Gravix
{
#ifdef GRAVIX_EDITOR_BUILD
	Ref<Shader> Shader::Create(const std::filesystem::path& shaderPath, ShaderType type)
	{
		Device* device = Application::Get().GetWindow().GetDevice();

		switch (device->GetType())
		{
		case DeviceType::None:    GX_VERIFY("DeviceType::None is currently not supported!"); return nullptr;
		case DeviceType::Vulkan:  return CreateRef<VulkanShader>(shaderPath, type);
		}
		GX_VERIFY("Unknown RendererAPI!");
		return nullptr;
	}
#endif

	Ref<Shader> Shader::Create(const std::filesystem::path& sourcePath, ShaderType type,
		const std::vector<std::vector<uint32_t>>& spirvData, const ShaderReflection& reflection)
	{
		Device* device = Application::Get().GetWindow().GetDevice();

		switch (device->GetType())
		{
		case DeviceType::None:    GX_VERIFY("DeviceType::None is currently not supported!"); return nullptr;
		case DeviceType::Vulkan:  return CreateRef<VulkanShader>(sourcePath, type, spirvData, reflection);
		}
		GX_VERIFY("Unknown RendererAPI!");
		return nullptr;
	}
}
