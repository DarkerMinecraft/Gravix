#pragma once

#include "Types/Framebuffer.h"
#include "Types/Texture.h"

#include <vector>

namespace Gravix 
{

	struct DeviceProperties
	{
		uint32_t Width;
		uint32_t Height;

		void* WindowHandle;

		bool VSync;
	};

	enum class DeviceType
	{
		None = 0,
		Vulkan = 1,
		DirectX12 = 2,
	};

	constexpr uint32_t FRAME_OVERLAP = 2;

	class Device
	{
	public:
		virtual ~Device() = default;

		virtual DeviceType GetType() const = 0;

		virtual void StartFrame() = 0;
		virtual void EndFrame() = 0;

		// Wait for all GPU operations to complete (use sparingly, only for cleanup/destruction)
		virtual void WaitIdle() = 0;

		void RegisterFramebuffer(Ref<Framebuffer> framebuffer) { m_Framebuffers.push_back(framebuffer); }
		std::vector<Ref<Framebuffer>>& GetFramebuffers() { return m_Framebuffers; }

		void RegisterTexture(Ref<Texture2D> texture) { m_Textures.push_back(texture); }
		std::vector<Ref<Texture2D>>& GetTextures() { return m_Textures; }
	private:
		std::vector<Ref<Framebuffer>> m_Framebuffers;
		std::vector<Ref<Texture2D>> m_Textures;
	};

}