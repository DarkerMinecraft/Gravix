#pragma once

#include "Renderer/Specification.h"

#include "Core/UUID.h"

#include "Asset/Asset.h"

#include <filesystem>

namespace Gravix
{

	/**
	 * @brief Texture specification for creation
	 */
	struct TextureSpecification
	{
		TextureFilter MinFilter = TextureFilter::Linear;
		TextureFilter MagFilter = TextureFilter::Linear;
		TextureWrap WrapS = TextureWrap::Repeat;
		TextureWrap WrapT = TextureWrap::Repeat;
		bool GenerateMipmaps = false;
		std::string DebugName = "Texture";
	};

	class Texture : public Asset
	{
	public:
		virtual ~Texture() = default;

		virtual uint32_t GetWidth() const = 0;
		virtual uint32_t GetHeight() const = 0;
		virtual uint32_t GetMipLevels() const = 0;

		virtual UUID GetUUID() = 0;

		virtual bool operator==(const Texture& other) const = 0;
	};

	class Texture2D : public Texture
	{
	public:
		virtual ~Texture2D() = default;

		static AssetType GetStaticType() { return AssetType::Texture2D; }
		virtual AssetType GetAssetType() const override { return GetStaticType(); }

		virtual void* GetImGuiAttachment() = 0;
		virtual void DestroyImGuiDescriptor() = 0;

		static Ref<Texture2D> Create(const std::filesystem::path& path, const TextureSpecification& specification = TextureSpecification());
		static Ref<Texture2D> Create(void* data, uint32_t width = 1, uint32_t height = 1, const TextureSpecification& specification = TextureSpecification());
	};

}