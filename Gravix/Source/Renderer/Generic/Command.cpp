#include "pch.h"
#include "Command.h"

#include "Core/Application.h"
#include "Renderer/Vulkan/VulkanCommandImpl.h"

namespace Gravix 
{

	Command::Command(const std::string& debugName)
	{
		Initialize(debugName);
	}

	Command::~Command()
	{
		delete m_Impl;
	}

	void Command::Initialize(const std::string& debugName)
	{
		Device* device = Application::Get().GetWindow().GetDevice();

		switch (device->GetType())
		{
		case DeviceType::None:    GX_STATIC_CORE_ASSERT("DeviceType::None is currently not supported!"); return;
		case DeviceType::Vulkan:  m_Impl = new VulkanCommandImpl(debugName); return;
		}
		GX_STATIC_CORE_ASSERT("Unknown RendererAPI!");
		return;
	}

}