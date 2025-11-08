#pragma once

#include "Asset/Asset.h"
#include "Asset/AssetMetadata.h"

#include "Renderer/Generic/Types/Texture.h"

namespace Gravix 
{

	class TextureImporter 
	{
	public:
		// AssetMetadata filepath is relative to asset directory
		static Ref<Texture2D> ImportTexture2D(AssetHandle handle, const AssetMetadata& metadata);

		// Load texture from file path (i.e. path has to absolute or relative to working directory)
		static Ref<Texture2D> LoadTexture2D(const std::filesystem::path& path);

		static Buffer LoadTexture2DToBuffer(const std::filesystem::path& metadata, int* width, int* height, int* channels);
	};

}