#include "pch.h"
#include "AssetImporter.h"

#include "Project/Project.h"

#include "Importers/TextureImporter.h"
#include "Importers/SceneImporter.h"
#include "Importers/ShaderImporter.h"
#include "Importers/PipelineImporter.h"
#include "Importers/MaterialImporter.h"

#include "Asset/AssetManager.h"

namespace Gravix
{

	static std::unordered_map<std::string, AssetType> s_ExtensionToAssetType =
	{
		{ ".png", AssetType::Texture2D },
		{ ".jpg", AssetType::Texture2D },
		{ ".jpeg", AssetType::Texture2D },
		{ ".bmp", AssetType::Texture2D },
		{ ".tga", AssetType::Texture2D },
		{ ".orbscene", AssetType::Scene },
		{ ".slang", AssetType::Shader },
		{ ".pipeline", AssetType::Pipeline },
		{ ".orbmat", AssetType::Material },
	};

	using AssetImportFunc = std::function<Ref<Asset>(AssetHandle, const AssetMetadata&)>;

	static std::unordered_map<AssetType, AssetImportFunc> s_AssetImportFunc =
	{
		{ AssetType::Texture2D, &TextureImporter::ImportTexture2D },
		{ AssetType::Scene, &SceneImporter::ImportScene },
		{ AssetType::Shader, &ShaderImporter::ImportShader },
		{ AssetType::Pipeline, &PipelineImporter::ImportPipeline },
		{ AssetType::Material, &MaterialImporter::ImportMaterial },
	};

	Ref<Asset> AssetImporter::ImportAsset(AssetHandle handle, const AssetMetadata& metadata)
	{
		if(AssetManager::IsAssetLoaded(handle))
			return Project::GetActive()->GetAssetManager()->GetAsset(handle);

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

		if (s_ExtensionToAssetType.contains(extension))
		{
			outMetadata->Type = s_ExtensionToAssetType.at(extension);
		}
		else
		{
			outMetadata->Type = AssetType::None;
			GX_CORE_WARN("Unsupported asset type for file: {0}", filePath.string());
		}

		outMetadata->LastModifiedTime = std::filesystem::last_write_time(Project::GetAssetDirectory() / filePath).time_since_epoch().count();

		return AssetHandle();
	}

}