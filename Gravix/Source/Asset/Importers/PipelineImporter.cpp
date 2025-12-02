#include "pch.h"
#include "PipelineImporter.h"

#include "Project/Project.h"
#include <yaml-cpp/yaml.h>
#include <fstream>

namespace Gravix
{

	// Helper functions to convert enums to/from strings
	static std::string BlendingToString(Blending blend)
	{
		switch (blend)
		{
		case Blending::None:             return "None";
		case Blending::Alpha:            return "Alpha";
		case Blending::Additive:         return "Additive";
		case Blending::Multiplicative:   return "Multiplicative";
		default:                         return "None";
		}
	}

	static Blending StringToBlending(const std::string& str)
	{
		if (str == "Alpha")            return Blending::Alpha;
		if (str == "Additive")         return Blending::Additive;
		if (str == "Multiplicative")   return Blending::Multiplicative;
		return Blending::None;
	}

	static std::string CompareOpToString(CompareOp op)
	{
		switch (op)
		{
		case CompareOp::Never:           return "Never";
		case CompareOp::Less:            return "Less";
		case CompareOp::Equal:           return "Equal";
		case CompareOp::LessOrEqual:     return "LessOrEqual";
		case CompareOp::Greater:         return "Greater";
		case CompareOp::NotEqual:        return "NotEqual";
		case CompareOp::GreaterOrEqual:  return "GreaterOrEqual";
		case CompareOp::Always:          return "Always";
		default:                         return "Less";
		}
	}

	static CompareOp StringToCompareOp(const std::string& str)
	{
		if (str == "Never")          return CompareOp::Never;
		if (str == "Less")           return CompareOp::Less;
		if (str == "Equal")          return CompareOp::Equal;
		if (str == "LessOrEqual")    return CompareOp::LessOrEqual;
		if (str == "Greater")        return CompareOp::Greater;
		if (str == "NotEqual")       return CompareOp::NotEqual;
		if (str == "GreaterOrEqual") return CompareOp::GreaterOrEqual;
		if (str == "Always")         return CompareOp::Always;
		return CompareOp::Less;
	}

	static std::string CullToString(Cull cull)
	{
		switch (cull)
		{
		case Cull::None:       return "None";
		case Cull::Front:      return "Front";
		case Cull::Back:       return "Back";
		case Cull::FrontBack:  return "FrontBack";
		default:               return "None";
		}
	}

	static Cull StringToCull(const std::string& str)
	{
		if (str == "Front")     return Cull::Front;
		if (str == "Back")      return Cull::Back;
		if (str == "FrontBack") return Cull::FrontBack;
		return Cull::None;
	}

	static std::string FrontFaceToString(FrontFace face)
	{
		switch (face)
		{
		case FrontFace::CounterClockwise: return "CounterClockwise";
		case FrontFace::Clockwise:        return "Clockwise";
		default:                          return "CounterClockwise";
		}
	}

	static FrontFace StringToFrontFace(const std::string& str)
	{
		if (str == "Clockwise") return FrontFace::Clockwise;
		return FrontFace::CounterClockwise;
	}

	static std::string FillToString(Fill fill)
	{
		switch (fill)
		{
		case Fill::Solid:      return "Solid";
		case Fill::Wireframe:  return "Wireframe";
		case Fill::Point:      return "Point";
		default:               return "Solid";
		}
	}

	static Fill StringToFill(const std::string& str)
	{
		if (str == "Wireframe") return Fill::Wireframe;
		if (str == "Point")     return Fill::Point;
		return Fill::Solid;
	}

	static std::string TopologyToString(Topology topology)
	{
		switch (topology)
		{
		case Topology::PointList:     return "PointList";
		case Topology::LineList:      return "LineList";
		case Topology::LineStrip:     return "LineStrip";
		case Topology::TriangleList:  return "TriangleList";
		case Topology::TriangleStrip: return "TriangleStrip";
		default:                      return "TriangleList";
		}
	}

	static Topology StringToTopology(const std::string& str)
	{
		if (str == "PointList")     return Topology::PointList;
		if (str == "LineList")      return Topology::LineList;
		if (str == "LineStrip")     return Topology::LineStrip;
		if (str == "TriangleList")  return Topology::TriangleList;
		if (str == "TriangleStrip") return Topology::TriangleStrip;
		return Topology::TriangleList;
	}

	Ref<Pipeline> PipelineImporter::ImportPipeline(AssetHandle handle, const AssetMetadata& metadata)
	{
		std::filesystem::path fullPath = Project::GetAssetDirectory() / metadata.FilePath;

		if (!std::filesystem::exists(fullPath))
		{
			GX_CORE_ERROR("Pipeline file not found: {0}", fullPath.string());
			return nullptr;
		}

		YAML::Node data = YAML::LoadFile(fullPath.string());

		if (!data["Pipeline"])
		{
			GX_CORE_ERROR("Invalid pipeline file: {0}", fullPath.string());
			return nullptr;
		}

		YAML::Node pipelineNode = data["Pipeline"];

		PipelineConfiguration config;

		// Blending
		if (pipelineNode["Blending"])
			config.BlendingMode = StringToBlending(pipelineNode["Blending"].as<std::string>());

		// Depth testing
		if (pipelineNode["DepthTest"])
			config.EnableDepthTest = pipelineNode["DepthTest"].as<bool>();
		if (pipelineNode["DepthWrite"])
			config.EnableDepthWrite = pipelineNode["DepthWrite"].as<bool>();
		if (pipelineNode["DepthCompareOp"])
			config.DepthCompareOp = StringToCompareOp(pipelineNode["DepthCompareOp"].as<std::string>());

		// Rasterization
		if (pipelineNode["CullMode"])
			config.CullMode = StringToCull(pipelineNode["CullMode"].as<std::string>());
		if (pipelineNode["FrontFace"])
			config.FrontFaceWinding = StringToFrontFace(pipelineNode["FrontFace"].as<std::string>());
		if (pipelineNode["FillMode"])
			config.FillMode = StringToFill(pipelineNode["FillMode"].as<std::string>());

		// Topology
		if (pipelineNode["Topology"])
			config.GraphicsTopology = StringToTopology(pipelineNode["Topology"].as<std::string>());

		// Line width
		if (pipelineNode["LineWidth"])
			config.LineWidth = pipelineNode["LineWidth"].as<float>();

		Ref<Pipeline> pipeline = CreateRef<Pipeline>(config);
		return pipeline;
	}

	void PipelineImporter::ExportPipeline(const std::filesystem::path& path, const Ref<Pipeline>& pipeline)
	{
		const PipelineConfiguration& config = pipeline->GetConfiguration();

		YAML::Emitter out;
		out << YAML::BeginMap;
		out << YAML::Key << "Pipeline" << YAML::Value << YAML::BeginMap;

		// Blending
		out << YAML::Key << "Blending" << YAML::Value << BlendingToString(config.BlendingMode);

		// Depth testing
		out << YAML::Key << "DepthTest" << YAML::Value << config.EnableDepthTest;
		out << YAML::Key << "DepthWrite" << YAML::Value << config.EnableDepthWrite;
		out << YAML::Key << "DepthCompareOp" << YAML::Value << CompareOpToString(config.DepthCompareOp);

		// Rasterization
		out << YAML::Key << "CullMode" << YAML::Value << CullToString(config.CullMode);
		out << YAML::Key << "FrontFace" << YAML::Value << FrontFaceToString(config.FrontFaceWinding);
		out << YAML::Key << "FillMode" << YAML::Value << FillToString(config.FillMode);

		// Topology
		out << YAML::Key << "Topology" << YAML::Value << TopologyToString(config.GraphicsTopology);

		// Line width
		out << YAML::Key << "LineWidth" << YAML::Value << config.LineWidth;

		out << YAML::EndMap; // Pipeline
		out << YAML::EndMap;

		std::ofstream fout(path);
		fout << out.c_str();
		fout.close();

		GX_CORE_INFO("Exported pipeline to: {0}", path.string());
	}

	Ref<Pipeline> PipelineImporter::CreateDefaultPipeline(const std::filesystem::path& path)
	{
		PipelineConfiguration config;
		// Default configuration is already set in the struct

		Ref<Pipeline> pipeline = CreateRef<Pipeline>(config);
		ExportPipeline(path, pipeline);

		return pipeline;
	}

}
