#pragma once

#include "Asset.h"
#include "AssetMetadata.h"

#include "Renderer/Generic/Types/Texture.h"

namespace Gravix 
{

	class TextureImporter 
	{
	public:
		Ref<Texture2D> ImportTexture2D(AssetHandle handle, const AssetMetadata& metadata);
	};

}