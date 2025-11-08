#include "pch.h"
#include "ProjectSerializer.h"

#include <yaml-cpp/yaml.h>
#include <fstream>

namespace Gravix 
{

	void ProjectSerializer::Serialize(const std::filesystem::path& path)
	{
		const auto& config = m_Project->GetConfig();

		YAML::Emitter out;
		out << YAML::BeginMap;
		out << YAML::Key << "Project" << YAML::Value;

		out << YAML::BeginMap;
		out << YAML::Key << "Name" << YAML::Value << config.Name;
		out << YAML::Key << "StartScene" << YAML::Value << config.StartScene.string();
		out << YAML::Key << "AssetDirectory" << YAML::Value << config.AssetDirectory.string();
		out << YAML::Key << "LibraryDirectory" << YAML::Value << config.LibraryDirectory.string();
		out << YAML::Key << "ScriptPath" << YAML::Value << config.ScriptPath.string();
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
		config.StartScene = projectNode["StartScene"].as<std::string>();
		config.AssetDirectory = projectNode["AssetDirectory"].as<std::string>();
		config.LibraryDirectory = projectNode["LibraryDirectory"].as<std::string>();
		config.ScriptPath = projectNode["ScriptPath"].as<std::string>();

		return true;
	}

}