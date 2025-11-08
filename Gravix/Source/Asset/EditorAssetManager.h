#pragma once

#include "AssetManagerBase.h"
#include "AssetMetadata.h"

namespace Gravix 
{

	using AssetRegistry = std::map<AssetHandle, AssetMetadata>;

	class EditorAssetManager : public AssetManagerBase
	{
	public:
		virtual Ref<Asset> GetAsset(AssetHandle handle) const override;

		virtual bool IsAssetLoaded(AssetHandle handle) const override;
		virtual bool IsAssetHandleValid(AssetHandle handle) const override;
		virtual AssetType GetAssetType(AssetHandle handle) const override;

		void ImportAsset(const std::filesystem::path& filePath);

		const AssetMetadata& GetAssetMetadata(AssetHandle handle) const;
		const AssetRegistry& GetAssetRegistry() const { return m_AssetRegistry; }

		const std::filesystem::path& GetAssetFilePath(AssetHandle handle) const;

		void ClearLoadedAssets() { m_LoadedAssets.clear(); }

		void SerializeAssetRegistry();
		void DeserializeAssetRegistry();
	private:
		AssetRegistry m_AssetRegistry;
		AssetMap m_LoadedAssets;
	};
}