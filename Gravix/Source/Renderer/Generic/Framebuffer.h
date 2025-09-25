#pragma once

#include <glm/glm.hpp>

namespace Gravix 
{
	
	enum class FramebufferTextureFormat
	{
		None = 0,
		// Color
		RGBA8 = 1,
		RGBA16F = 2,
		RGBA32F = 3 ,
		RGBA32UI = 4,
		// Depth/stencil
		DEPTH24STENCIL8 = 10,
		DEPTH32FSTENCIL8 = 11,
		// Defaults
		Default = RGBA8,
		Depth = DEPTH24STENCIL8
	};

	struct FramebufferSpecification
	{
		uint32_t Width = 0, Height = 0;
		bool Multisampled = false;

		std::vector<FramebufferTextureFormat> Attachments;
	};

	class Framebuffer 
	{
	public:
		virtual ~Framebuffer() = default;

		virtual uint32_t GetWidth() const = 0;
		virtual uint32_t GetHeight() const = 0;

		virtual void SetClearColor(uint32_t index, const glm::vec4 clearColor) = 0;

		virtual void* GetColorAttachmentID(uint32_t index) = 0;

		virtual void Resize(uint32_t width, uint32_t height) = 0;
		virtual void DestroyImGuiDescriptors() = 0;

		static Ref<Framebuffer> Create(const FramebufferSpecification& spec);
	};

}