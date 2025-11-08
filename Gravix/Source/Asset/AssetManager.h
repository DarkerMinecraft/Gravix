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
		static Ref<T> GetAsset(AssetHandle handle) 
		{
			return Project::GetActive()->GetAssetManager()->GetAsset<T>(handle);
		}
	};
}