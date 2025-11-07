#include "pch.h"
#include "EditorAssetManager.h"

#include "AssetImporter.h"

namespace Gravix 
{

	bool EditorAssetManager::IsAssetHandleValid(AssetHandle handle) const
	{
		return handle != 0 && m_AssetRegistry.contains(handle);
	}

	const AssetMetadata& EditorAssetManager::GetAssetMetadata(AssetHandle handle) const
	{
		static AssetMetadata invalidMetadata{};
		if(m_AssetRegistry.contains(handle))
			return m_AssetRegistry.at(handle);

		return invalidMetadata;
	}

	bool EditorAssetManager::IsAssetLoaded(AssetHandle handle) const
	{
		return m_LoadedAssets.contains(handle);
	}

	Ref<Asset> EditorAssetManager::GetAsset(AssetHandle handle) const
	{
		if(!IsAssetHandleValid(handle))
			return nullptr;

		Ref<Asset> asset;
		if (IsAssetLoaded(handle))
		{
			asset = m_LoadedAssets.at(handle);
		} 
		else
		{
			const AssetMetadata& metadata = GetAssetMetadata(handle);
			asset = AssetImporter::ImportAsset(handle, metadata);
			if (!asset) 
			{
				GX_CORE_ERROR("Failed to load asset: {0}", metadata.FilePath.string());
				return nullptr;
			}
		}
		return asset;
	}

}