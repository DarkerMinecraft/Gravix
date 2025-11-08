#include "pch.h"
#include "AssetImporter.h"

#include "Project/Project.h"
#include "Importers/TextureImporter.h"

namespace Gravix 
{

	using AssetImportFunc = std::function<Ref<Asset>(AssetHandle, const AssetMetadata&)>;

	static std::unordered_map<AssetType, AssetImportFunc> s_AssetImportFunc = 
	{
		{ AssetType::Texture2D, &TextureImporter::ImportTexture2D }
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

	AssetHandle AssetImporter::GenerateAssetHandle(const std::filesystem::path& filePath, AssetMetadata* outMetadata)
	{
		outMetadata->FilePath = filePath;
		std::string extension = filePath.extension().string();

		if (extension == ".png" || extension == ".jpg" || extension == ".jpeg" || extension == ".bmp" || extension == ".tga")
		{
			outMetadata->Type = AssetType::Texture2D;
		}
		else
		{
			outMetadata->Type = AssetType::None;
			GX_CORE_WARN("Unsupported asset type for file: {0}", filePath.string());
		}

		return AssetHandle();
	}

}