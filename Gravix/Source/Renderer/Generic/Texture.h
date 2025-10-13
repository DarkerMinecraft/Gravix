#pragma once

#include "Renderer/Specification.h"

#include <filesystem>

namespace Gravix
{

	/**
	 * @brief Texture specification for creation
	 */
	struct TextureSpecification
	{
		uint32_t Width = 1;
		uint32_t Height = 1;
		TextureFilter MinFilter = TextureFilter::Linear;
		TextureFilter MagFilter = TextureFilter::Linear;
		TextureWrap WrapS = TextureWrap::Repeat;
		TextureWrap WrapT = TextureWrap::Repeat;
		bool GenerateMipmaps = true;
		std::string DebugName = "Texture";
	};

	class Texture
	{
	public:
		virtual ~Texture() = default;

		virtual uint32_t GetWidth() const = 0;
		virtual uint32_t GetHeight() const = 0;
		virtual uint32_t GetMipLevels() const = 0;
	};

	class Texture2D : public Texture
	{
	public:
		virtual ~Texture2D() = default;

		static Ref<Texture2D> Create(const std::filesystem::path& path, const TextureSpecification& specification = TextureSpecification());
		static Ref<Texture2D> Create(void* data, uint32_t width = 1, uint32_t height = 1, const TextureSpecification& specification = TextureSpecification());
	};

}