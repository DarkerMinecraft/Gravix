#pragma once

#include "Asset.h"
#include <filesystem>

namespace Gravix 
{

	struct AssetMetadata
	{
		AssetType Type = AssetType::None;
		std::filesystem::path FilePath;
		uint64_t LastModifiedTime = 0;

		operator bool() const { return Type != AssetType::None; }
	};

}