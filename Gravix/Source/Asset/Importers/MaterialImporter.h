#pragma once

#include "Asset/Asset.h"
#include "Asset/AssetMetadata.h"

#include "Renderer/Generic/Types/Material.h"
#include "Renderer/Generic/Types/Framebuffer.h"

namespace Gravix
{

	class MaterialImporter
	{
	public:
		// Import material from .orbmat YAML file
		// Loads Shader and Pipeline assets before creating Material
		// Note: Call SetFramebuffer() on the material before using it for rendering
		static Ref<Material> ImportMaterial(AssetHandle handle, const AssetMetadata& metadata);

		// Export material to .orbmat YAML file
		static void ExportMaterial(const std::filesystem::path& path, AssetHandle shaderHandle, AssetHandle pipelineHandle);

		// Create default material and save it
		static void CreateDefaultMaterial(const std::filesystem::path& path, AssetHandle shaderHandle, AssetHandle pipelineHandle);
	};

}
