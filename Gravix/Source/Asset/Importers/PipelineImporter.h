#pragma once

#include "Asset/Asset.h"
#include "Asset/AssetMetadata.h"
#include "Renderer/Generic/Types/Pipeline.h"

namespace Gravix
{

	class PipelineImporter
	{
	public:
		// Import pipeline from YAML file
		static Ref<Pipeline> ImportPipeline(AssetHandle handle, const AssetMetadata& metadata);

		// Export pipeline to YAML file
		static void ExportPipeline(const std::filesystem::path& path, const Ref<Pipeline>& pipeline);

		// Create default pipeline and save it
		static Ref<Pipeline> CreateDefaultPipeline(const std::filesystem::path& path);
	};

}
