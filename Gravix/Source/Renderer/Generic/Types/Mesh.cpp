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
		case DeviceType::None:    GX_VERIFY("DeviceType::None is currently not supported!"); return nullptr;
		case DeviceType::Vulkan: return CreateRef<VulkanMesh>(device, vertexSize);
		}
		GX_VERIFY("Unknown RendererAPI!");
		return nullptr;
	}

}