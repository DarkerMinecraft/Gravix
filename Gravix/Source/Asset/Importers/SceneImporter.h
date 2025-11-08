#pragma once

#include "Asset/Asset.h"
#include "Asset/AssetMetadata.h"

namespace Gravix 
{
	class SceneImporter 
	{
	public:
		static Ref<Asset> ImportScene(AssetHandle handle, const AssetMetadata& metadata);

		static YAML::Node LoadSceneToYAML(const std::filesystem::path& path, std::vector<AssetHandle>* outDependencies = nullptr);
	};
}