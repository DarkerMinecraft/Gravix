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

	std::string_view AssetTypeToString(AssetType type);
	AssetType StringToAssetType(const std::string& typeStr);

	class Asset 
	{
	public:
		virtual AssetType GetAssetType() const = 0;
	private:
		AssetHandle m_Handle;
	};

}