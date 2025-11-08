#pragma once

#include "AssetManagerBase.h"
#include "Project/Project.h"

#include "Asset.h"

namespace Gravix 
{
	class AssetManager 
	{
	public:
		template<typename TAsset> 
		requires std::is_base_of_v<Asset, TAsset>
		static Ref<TAsset> GetAsset(AssetHandle handle) { return Project::GetActive()->GetAssetManager()->GetAsset<TAsset>(handle); }

		static bool IsAssetLoaded(AssetHandle handle) { return Project::GetActive()->GetAssetManager()->IsAssetLoaded(handle); }
		static bool IsValidAssetHandle(AssetHandle handle) { return Project::GetActive()->GetAssetManager()->IsAssetHandleValid(handle); }
		static AssetType GetAssetType(AssetHandle handle) { return Project::GetActive()->GetAssetManager()->GetAssetType(handle); }
	};
}