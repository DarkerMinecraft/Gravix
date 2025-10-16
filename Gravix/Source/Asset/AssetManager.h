#pragma once

#include "AssetManagerBase.h";

namespace Gravix 
{
	class AssetManager 
	{
	public:
		template<typename T>
		requires(std::is_base_of<Asset, T>::value)
		static Ref<T> GetAsset(AssetHandle handle);
	};
}