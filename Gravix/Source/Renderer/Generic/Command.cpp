#include "pch.h"
#include "Command.h"

#include "Core/Application.h"
#include "Renderer/Vulkan/VulkanCommandImpl.h"

namespace Gravix 
{

	Command::Command(Ref<Framebuffer> framebuffer, uint32_t presentIndex, bool shouldCopy)
	{
		Initialize(framebuffer, presentIndex, shouldCopy);
	}

	Command::~Command()
	{
		delete m_Impl;
	}

	void Command::BeginRendering()
	{
		if(m_Impl)
			m_Impl->BeginRendering();
	}

	void Command::DrawImGui()
	{
		if(m_Impl)
			m_Impl->DrawImGui();
	}

	void Command::EndRendering()
	{
		if(m_Impl)
			m_Impl->EndRendering();
	}

	void Command::Initialize(Ref<Framebuffer> framebuffer, uint32_t presentIndex, bool shouldCopy)
	{
		Device* device = Application::Get().GetWindow().GetDevice();

		switch (device->GetType())
		{
		case DeviceType::None:    GX_STATIC_CORE_ASSERT("DeviceType::None is currently not supported!"); return;
		case DeviceType::Vulkan:  m_Impl = new VulkanCommandImpl(device, framebuffer, presentIndex, shouldCopy); return;
		}
		GX_STATIC_CORE_ASSERT("Unknown RendererAPI!");
		return;
	}

}