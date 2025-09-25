#include "pch.h"
#include "Framebuffer.h"

#include "Core/Application.h"

#include "Renderer/Vulkan/VulkanFramebuffer.h"

namespace Gravix 
{

	Ref<Framebuffer> Framebuffer::Create(const FramebufferSpecification& spec)
	{
		Device* device = Application::Get().GetWindow().GetDevice();

		switch (device->GetType())
		{
			case DeviceType::None:    GX_STATIC_CORE_ASSERT("DeviceType::None is currently not supported!"); return nullptr;
			case DeviceType::Vulkan: {
				Ref<VulkanFramebuffer> framebuffer = CreateRef<VulkanFramebuffer>(device, spec);
				device->RegisterFramebuffer(framebuffer);

				return framebuffer;
			}
		}
		GX_STATIC_CORE_ASSERT("Unknown RendererAPI!");
		return nullptr;
	}
	
}