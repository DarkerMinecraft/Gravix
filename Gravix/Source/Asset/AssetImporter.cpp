#include "pch.h"
#include "AssetImporter.h"

#include "Importers/TextureImporter.h"

namespace Gravix 
{

	using AssetImportFunc = std::function<Ref<Asset>(AssetHandle, const AssetMetadata&)>;
	static std::unordered_map<AssetType, AssetImportFunc> s_AssetImportFunc = 
	{
		{AssetType::Texture2D, TextureImporter::ImportTexture2D }
	};

	Ref<Asset> AssetImporter::ImportAsset(AssetHandle handle, const AssetMetadata& metadata)
	{
		if (!s_AssetImportFunc.contains(metadata.Type))
		{
			GX_CORE_ERROR("No importer found for asset type: {0}", static_cast<int>(metadata.Type));
			return nullptr;
		}

		return s_AssetImportFunc.at(metadata.Type)(handle, metadata);
	}

}