#pragma once

#include "Asset/Asset.h"
#include "Asset/AssetMetadata.h"

namespace Gravix 
{
	class SceneImporter 
	{
	public:
		static Ref<Asset> ImportScene(AssetHandle handle, const AssetMetadata& metadata);
	};
}