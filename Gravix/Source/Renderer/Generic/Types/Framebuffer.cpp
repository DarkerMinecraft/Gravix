#include "pch.h"
#include "Framebuffer.h"

#include "Core/Application.h"

#include "Renderer/Vulkan/Types/VulkanFramebuffer.h"

namespace Gravix 
{

	Ref<Framebuffer> Framebuffer::Create(const FramebufferSpecification& spec)
	{
		Device* device = Application::Get().GetWindow().GetDevice();

		switch (device->GetType())
		{
			case DeviceType::None:    GX_VERIFY("DeviceType::None is currently not supported!"); return nullptr;
			case DeviceType::Vulkan: {
				Ref<VulkanFramebuffer> framebuffer = CreateRef<VulkanFramebuffer>(device, spec);
				device->RegisterFramebuffer(framebuffer);

				return framebuffer;
			}
		}
		GX_VERIFY("Unknown RendererAPI!");
		return nullptr;
	}
	
}