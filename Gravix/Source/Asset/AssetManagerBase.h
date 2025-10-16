#pragma once

#include "Asset.h"

namespace Gravix 
{

	class AssetManagerBase 
	{

	public:
		virtual Ref<Asset> GetAsset(AssetHandle handle) const = 0;
		virtual void AddAsset(Ref<Asset> asset) = 0;
	};
}