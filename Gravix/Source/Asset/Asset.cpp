#include "pch.h"
#include "Asset.h"

namespace Gravix
{

	std::string_view AssetTypeToString(AssetType type)
	{	
		switch (type)
		{
		case AssetType::None:      return "None";
		case AssetType::Scene:     return "Scene";
		case AssetType::Texture2D: return "Texture2D";
		case AssetType::Material:  return "Material";
		case AssetType::Script:    return "Script";
		default:                   return "Unknown";
		}
	
	}

	AssetType StringToAssetType(const std::string& typeStr)
	{
		if (typeStr == "None")       return AssetType::None;
		if (typeStr == "Scene")      return AssetType::Scene;
		if (typeStr == "Texture2D")  return AssetType::Texture2D;
		if (typeStr == "Material")   return AssetType::Material;
		if (typeStr == "Script")     return AssetType::Script;
		return AssetType::None;
	}

}

