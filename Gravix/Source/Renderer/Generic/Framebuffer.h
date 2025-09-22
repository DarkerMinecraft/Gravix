#pragma once

namespace Gravix 
{
	
	enum class FramebufferTextureFormat
	{
		None = 0,
		// Color
		RGBA8,
		RGBA16F,
		RGBA32F,
		RGBA32UI,
		// Depth/stencil
		DEPTH24STENCIL8,
		DEPTH32FSTENCIL8,
		// Defaults
		Default = RGBA8,
		Depth = DEPTH24STENCIL8
	};

	struct FramebufferSpecification
	{
		const std::string DebugName = "Framebuffer";
		uint32_t Width = -1, Height = -1;
		bool SwapchainTarget = false;
		bool Multisampled = false;

		std::vector<FramebufferTextureFormat> Attachments;
	};

	class Framebuffer 
	{
	public:
		virtual ~Framebuffer() = default;

		static Ref<Framebuffer> Create(const FramebufferSpecification& spec);
	};

}