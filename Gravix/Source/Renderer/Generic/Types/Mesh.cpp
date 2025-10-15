#include "pch.h"
#include "Mesh.h"

#include "Core/Application.h"

#include "Renderer/Vulkan/Types/VulkanMesh.h"

namespace Gravix 
{

	Ref<Mesh> Mesh::Create(size_t vertexSize)
	{
		Device* device = Application::Get().GetWindow().GetDevice();

		switch (device->GetType())
		{
		case DeviceType::None:    GX_STATIC_CORE_ASSERT("DeviceType::None is currently not supported!"); return nullptr;
		case DeviceType::Vulkan: return CreateRef<VulkanMesh>(device, vertexSize);
		}
		GX_STATIC_CORE_ASSERT("Unknown RendererAPI!");
		return nullptr;
	}

}