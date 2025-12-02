#include "pch.h"
#include "MaterialImporter.h"

#include "Renderer/Generic/Types/Shader.h"
#include "Renderer/Generic/Types/Pipeline.h"

#include "Asset/EditorAssetManager.h"
#include "Project/Project.h"

#include <yaml-cpp/yaml.h>
#include <fstream>

namespace Gravix
{

	Ref<Material> MaterialImporter::ImportMaterial(AssetHandle handle, const AssetMetadata& metadata)
	{
		std::filesystem::path fullPath = Project::GetAssetDirectory() / metadata.FilePath;

		if (!std::filesystem::exists(fullPath))
		{
			GX_CORE_ERROR("Material file not found: {0}", fullPath.string());
			return nullptr;
		}

		YAML::Node data = YAML::LoadFile(fullPath.string());

		if (!data["Material"])
		{
			GX_CORE_ERROR("Invalid material file: {0}", fullPath.string());
			return nullptr;
		}

		YAML::Node materialNode = data["Material"];

		// Read Shader and Pipeline asset handles
		if (!materialNode["Shader"] || !materialNode["Pipeline"])
		{
			GX_CORE_ERROR("Material file missing Shader or Pipeline reference: {0}", fullPath.string());
			return nullptr;
		}

		AssetHandle shaderHandle = materialNode["Shader"].as<uint64_t>();
		AssetHandle pipelineHandle = materialNode["Pipeline"].as<uint64_t>();

		// Load Shader and Pipeline assets through AssetManager
		// This ensures they are compiled/loaded before the Material is created
		auto assetManager = Project::GetActive()->GetEditorAssetManager();

		Ref<Shader> shader = assetManager->GetAsset(shaderHandle);
		if (!shader)
		{
			GX_CORE_ERROR("Failed to load shader for material: {0}", fullPath.string());
			return nullptr;
		}

		Ref<Pipeline> pipeline = assetManager->GetAsset(pipelineHandle);
		if (!pipeline)
		{
			GX_CORE_ERROR("Failed to load pipeline for material: {0}", fullPath.string());
			return nullptr;
		}

		// Create Material with loaded Shader and Pipeline
		// Note: Pipeline will be built when SetFramebuffer() is called
		Ref<Material> material = Material::Create(shaderHandle, pipelineHandle);

		if (!material)
		{
			GX_CORE_ERROR("Failed to create material: {0}", fullPath.string());
			return nullptr;
		}

		GX_CORE_INFO("Imported material: {0} (call SetFramebuffer before rendering)", fullPath.string());
		return material;
	}

	void MaterialImporter::ExportMaterial(const std::filesystem::path& path, AssetHandle shaderHandle, AssetHandle pipelineHandle)
	{
		YAML::Emitter out;
		out << YAML::BeginMap;
		out << YAML::Key << "Material" << YAML::Value << YAML::BeginMap;

		// Write Shader and Pipeline asset handles
		out << YAML::Key << "Shader" << YAML::Value << (uint64_t)shaderHandle;
		out << YAML::Key << "Pipeline" << YAML::Value << (uint64_t)pipelineHandle;

		out << YAML::EndMap; // Material
		out << YAML::EndMap;

		std::ofstream fout(path);
		fout << out.c_str();
		fout.close();

		GX_CORE_INFO("Exported material to: {0}", path.string());
	}

	void MaterialImporter::CreateDefaultMaterial(const std::filesystem::path& path, AssetHandle shaderHandle, AssetHandle pipelineHandle)
	{
		ExportMaterial(path, shaderHandle, pipelineHandle);
	}

}
