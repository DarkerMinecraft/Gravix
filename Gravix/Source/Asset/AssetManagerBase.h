#pragma once

#include "Asset.h"

#include <unordered_map>
#include <map>

namespace Gravix 
{

	using AssetMap = std::unordered_map<AssetHandle, Ref<Asset>>;

	class AssetManagerBase 
	{
	public:
		virtual Ref<Asset> GetAsset(AssetHandle handle) const = 0;

		virtual bool IsAssetHandleValid(AssetHandle handle) const = 0;
		virtual bool IsAssetLoaded(AssetHandle handle) const = 0;
	};
}