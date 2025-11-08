#pragma once

#include "AssetManagerBase.h"
#include "Project/Project.h"

namespace Gravix 
{
	class AssetManager 
	{
	public:
		template<typename T>
		requires(std::is_base_of<Asset, T>::value)
		static Ref<T> GetAsset(AssetHandle handle) { return Project::GetActive()->GetAssetManager()->GetAsset<T>(handle); }

		static bool IsAssetLoaded(AssetHandle handle) { return Project::GetActive()->GetAssetManager()->IsAssetLoaded(handle); }
		static bool IsValidAssetHandle(AssetHandle handle) { return Project::GetActive()->GetAssetManager()->IsAssetHandleValid(handle); }
		static AssetType GetAssetType(AssetHandle handle) { return Project::GetActive()->GetAssetManager()->GetAssetType(handle); }
	};
}