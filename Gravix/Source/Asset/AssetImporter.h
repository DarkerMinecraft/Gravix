#pragma once

#include "Asset.h"
#include "AssetMetadata.h"

#include "Core/Buffer.h"

namespace Gravix 
{

	class AssetImporter
	{
	public:
		static Ref<Asset> ImportAsset(AssetHandle handle, const AssetMetadata& metadata);
		static AssetHandle GenerateAssetHandle(const std::filesystem::path& filePath, AssetMetadata* outMetadata);
	};

}