#pragma once

#include "Core/UUID.h"
#include "Core/RefCounted.h"

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

	enum AssetState
	{
		NotLoaded,
		Loading,
		ReadyForGPU,
		Loaded,
		Failed
	};

	std::string_view AssetTypeToString(AssetType type);
	AssetType StringToAssetType(const std::string& typeStr);

	class Asset : public RefCounted
	{
	public:
		virtual ~Asset() = default;
		virtual AssetType GetAssetType() const = 0;
	private:
		AssetHandle m_Handle;
	};

}