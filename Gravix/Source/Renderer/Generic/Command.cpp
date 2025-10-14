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

	void Command::SetActiveMaterial(Material* material)
	{
		if(m_Impl)
			m_Impl->SetActiveMaterial(material);
	}

	void Command::BindResource(uint32_t binding, Framebuffer* buffer, uint32_t index, bool sampler /*= false*/)
	{
		if(m_Impl)
			m_Impl->BindResource(binding, buffer, index, sampler);
	}

	void Command::BindResource(uint32_t binding, uint32_t index, Texture2D* texture)
	{
		if(m_Impl)
			m_Impl->BindResource(binding, index, texture);
	}

	void Command::BindMaterial(void* pushConstants /*= nullptr*/)
	{
		if(m_Impl)
			m_Impl->BindMaterial(pushConstants);
	}

	void Command::Dispatch()
	{
		if(m_Impl)
			m_Impl->Dispatch();
	}

	void Command::BeginRendering()
	{
		if(m_Impl)
			m_Impl->BeginRendering();
	}

	void Command::BindMesh(Mesh* mesh)
	{
		if(m_Impl)
			m_Impl->BindMesh(mesh);
	}

	void Command::Draw(uint32_t vertexCount, uint32_t instanceCount /*= 1*/, uint32_t firstVertex /*= 0*/, uint32_t firstInstance /*= 0*/)
	{
		if(m_Impl)
			m_Impl->Draw(vertexCount, instanceCount, firstVertex, firstInstance);
	}

	void Command::DrawIndexed(uint32_t indexCount, uint32_t instanceCount /*= 1*/, uint32_t firstIndex /*= 0*/, int32_t vertexOffset /*= 0*/, uint32_t firstInstance /*= 0*/)
	{
		if(m_Impl)
			m_Impl->DrawIndexed(indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
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