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
		case AssetType::Shader:    return "Shader";
		case AssetType::Pipeline:  return "Pipeline";
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
		if (typeStr == "Shader")     return AssetType::Shader;
		if (typeStr == "Pipeline")   return AssetType::Pipeline;
		return AssetType::None;
	}

}

