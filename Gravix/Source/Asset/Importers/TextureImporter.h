#pragma once

#include "Asset/Asset.h"
#include "Asset/AssetMetadata.h"

#include "Renderer/Generic/Types/Texture.h"

namespace Gravix 
{

	class TextureImporter 
	{
	public:
		static Ref<Texture2D> ImportTexture2D(AssetHandle handle, const AssetMetadata& metadata);
	};

}