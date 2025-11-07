#pragma once

#include "Asset.h"
#include "AssetMetadata.h"

namespace Gravix 
{

	class AssetImporter
	{
	public:
		static Ref<Asset> ImportAsset(AssetHandle handle, const AssetMetadata& metadata);
	}

}