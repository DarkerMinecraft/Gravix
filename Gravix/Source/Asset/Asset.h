#pragma once

#include "Core/UUID.h"

namespace Gravix 
{

	using AssetHandle = UUID;

	enum class AssetType
	{
		None = 0,
		Scene,
		Texture2D,
		Material,
		Script,
	};

	class Asset 
	{
	public:
		virtual AssetType GetAssetType() const = 0;
	private:
		AssetHandle m_Handle;
	};

}