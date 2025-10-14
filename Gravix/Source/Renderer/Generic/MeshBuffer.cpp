#include "pch.h"
#include "MeshBuffer.h"

#include "Core/Application.h"

#include "Renderer/Vulkan/VulkanMeshBuffer.h"

namespace Gravix 
{

	Ref<MeshBuffer> MeshBuffer::Create(ReflectedStruct vertexLayout, uint32_t maxVertices, uint32_t maxIndices)
	{
		Device* device = Application::Get().GetWindow().GetDevice();

		switch (device->GetType())
		{
		case DeviceType::None:    GX_STATIC_CORE_ASSERT("DeviceType::None is currently not supported!"); return nullptr;
		case DeviceType::Vulkan: return CreateRef<VulkanMeshBuffer>(device, vertexLayout, maxVertices, maxIndices);
		}
		GX_STATIC_CORE_ASSERT("Unknown RendererAPI!");
		return nullptr;
	}

}