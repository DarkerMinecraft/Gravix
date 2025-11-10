#include "pch.h"
#include "ProjectSerializer.h"
#include "Serialization/YAMLConverters.h"

#include <yaml-cpp/yaml.h>
#include <fstream>

namespace Gravix 
{

	void ProjectSerializer::Serialize(const std::filesystem::path& path)
	{
		const auto& config = m_Project->GetConfig();

		// Get the project directory (where the .orbproj file will be saved)
		std::filesystem::path projectDirectory = path.parent_path();

		// Convert absolute paths to relative paths for portability
		std::filesystem::path relativeAssetDir = std::filesystem::relative(config.AssetDirectory, projectDirectory);
		std::filesystem::path relativeLibraryDir = std::filesystem::relative(config.LibraryDirectory, projectDirectory);
		std::filesystem::path relativeScriptPath = std::filesystem::relative(config.ScriptPath, projectDirectory);

		YAML::Emitter out;
		out << YAML::BeginMap;
		out << YAML::Key << "Project" << YAML::Value;

		out << YAML::BeginMap;
		out << YAML::Key << "Name" << YAML::Value << config.Name;
		out << YAML::Key << "StartScene" << YAML::Value << (uint64_t)config.StartScene;
		out << YAML::Key << "AssetDirectory" << YAML::Value << relativeAssetDir.string();
		out << YAML::Key << "LibraryDirectory" << YAML::Value << relativeLibraryDir.string();
		out << YAML::Key << "ScriptPath" << YAML::Value << relativeScriptPath.string();

		// Serialize Physics Settings
		out << YAML::Key << "Physics" << YAML::BeginMap;
		out << YAML::Key << "Gravity" << YAML::Value << config.Physics.Gravity;
		out << YAML::Key << "RestitutionThreshold" << YAML::Value << config.Physics.RestitutionThreshold;
		out << YAML::EndMap;

		out << YAML::EndMap;

		out << YAML::EndMap;

		std::ofstream fout(path);
		fout << out.c_str();
	}

	bool ProjectSerializer::Deserialize(const std::filesystem::path& path)
	{
		auto& config = m_Project->GetConfig();

		YAML::Node data;
		try
		{
			data = YAML::LoadFile(path.string());
		} catch(YAML::BadFile& e) 
		{
			GX_CORE_ERROR("Failed to load project file: {0}", path.string());
			return false;
		}
		auto projectNode = data["Project"];
		if (!projectNode)
		{
			GX_CORE_ERROR("Failed to load project file: {0}", path.string());
			return false;
		}

		config.Name = projectNode["Name"].as<std::string>();
		config.StartScene = (AssetHandle)projectNode["StartScene"].as<uint64_t>();
		config.AssetDirectory = projectNode["AssetDirectory"].as<std::string>();
		config.LibraryDirectory = projectNode["LibraryDirectory"].as<std::string>();
		config.ScriptPath = projectNode["ScriptPath"].as<std::string>();

		// Deserialize Physics Settings (with defaults if not present)
		auto physicsNode = projectNode["Physics"];
		if (physicsNode)
		{
			if (physicsNode["Gravity"])
				config.Physics.Gravity = physicsNode["Gravity"].as<glm::vec2>();
			if (physicsNode["RestitutionThreshold"])
				config.Physics.RestitutionThreshold = physicsNode["RestitutionThreshold"].as<float>();
		}

		return true;
	}

}