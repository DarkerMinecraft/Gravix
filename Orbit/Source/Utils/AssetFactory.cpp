#include "AssetFactory.h"

#include "Core/Log.h"
#include "Scene/Scene.h"
#include "Serialization/Scene/SceneSerializer.h"
#include "Project/Project.h"
#include "Asset/EditorAssetManager.h"
#include "Asset/Importers/PipelineImporter.h"
#include "Asset/Importers/MaterialImporter.h"
#include "Renderer/Generic/Types/Pipeline.h"

#include <fstream>

namespace Gravix
{

	std::string AssetFactory::GenerateUniqueFileName(const std::filesystem::path& directory, const std::string& baseName, const std::string& extension)
	{
		std::filesystem::path filePath = directory / (baseName + extension);
		int counter = 1;

		while (std::filesystem::exists(filePath))
		{
			filePath = directory / (baseName + std::to_string(counter++) + extension);
		}

		return filePath.filename().string();
	}

	std::string AssetFactory::GetScriptTemplate(const std::string& className)
	{
		return "using System;\n"
			   "using GravixEngine;\n"
			   "\n"
			   "public class " + className + " : Entity\n"
			   "{\n"
			   "    public void OnCreate()\n"
			   "    {\n"
			   "        \n"
			   "    }\n"
			   "\n"
			   "    public void OnUpdate(float deltaTime)\n"
			   "    {\n"
			   "        \n"
			   "    }\n"
			   "}\n";
	}

	std::string AssetFactory::GetGraphicsShaderTemplate()
	{
		return "// Graphics Shader\n"
			   "// This shader contains vertex and fragment entry points for rendering\n"
			   "\n"
			   "struct VertexInput\n"
			   "{\n"
			   "    float3 position : POSITION;\n"
			   "    float2 uv : TEXCOORD0;\n"
			   "    float4 color : COLOR;\n"
			   "};\n"
			   "\n"
			   "struct VertexOutput\n"
			   "{\n"
			   "    float4 position : SV_Position;\n"
			   "    float2 uv : TEXCOORD0;\n"
			   "    float4 color : COLOR;\n"
			   "};\n"
			   "\n"
			   "struct PushConstants\n"
			   "{\n"
			   "    float4x4 viewProjMatrix;\n"
			   "};\n"
			   "\n"
			   "[[vk::push_constant]] PushConstants pushConstants;\n"
			   "\n"
			   "[shader(\"vertex\")]\n"
			   "VertexOutput vertexMain(VertexInput input)\n"
			   "{\n"
			   "    VertexOutput output;\n"
			   "    output.position = mul(pushConstants.viewProjMatrix, float4(input.position, 1.0));\n"
			   "    output.uv = input.uv;\n"
			   "    output.color = input.color;\n"
			   "    return output;\n"
			   "}\n"
			   "\n"
			   "[shader(\"fragment\")]\n"
			   "float4 fragmentMain(VertexOutput input) : SV_Target\n"
			   "{\n"
			   "    return input.color;\n"
			   "}\n";
	}

	std::string AssetFactory::GetComputeShaderTemplate()
	{
		return "// Compute Shader\n"
			   "// This shader performs parallel computations on the GPU\n"
			   "\n"
			   "struct PushConstants\n"
			   "{\n"
			   "    uint width;\n"
			   "    uint height;\n"
			   "};\n"
			   "\n"
			   "[[vk::push_constant]] PushConstants pushConstants;\n"
			   "\n"
			   "[shader(\"compute\")]\n"
			   "[numthreads(8, 8, 1)]\n"
			   "void computeMain(uint3 threadID : SV_DispatchThreadID)\n"
			   "{\n"
			   "    // Add your compute logic here\n"
			   "    // threadID contains the global thread coordinates\n"
			   "}\n";
	}

	bool AssetFactory::CreateScene(const std::filesystem::path& directory)
	{
		try
		{
			// Ensure the directory exists
			if (!std::filesystem::exists(directory))
				std::filesystem::create_directories(directory);

			// Generate unique filename
			std::string filename = GenerateUniqueFileName(directory, "NewScene", ".orbscene");
			std::filesystem::path scenePath = directory / filename;

			// Create an empty scene
			Ref<Scene> newScene = CreateRef<Scene>();

			// Serialize the scene
			SceneSerializer serializer(newScene);
			serializer.Serialize(scenePath);

			// Import into asset manager
			auto assetDirectory = Project::GetAssetDirectory();
			auto relativePath = std::filesystem::relative(scenePath, assetDirectory);
			auto assetManager = Project::GetActive()->GetEditorAssetManager();
			assetManager->ImportAsset(relativePath);
			assetManager->SerializeAssetRegistry();

			GX_CORE_INFO("Created new scene: {0}", filename);
			return true;
		}
		catch (const std::exception& e)
		{
			GX_CORE_ERROR("Failed to create scene: {0}", e.what());
			return false;
		}
	}

	bool AssetFactory::CreateScript(const std::filesystem::path& directory)
	{
		try
		{
			// Ensure the directory exists
			if (!std::filesystem::exists(directory))
				std::filesystem::create_directories(directory);

			// Generate unique filename
			std::string filename = GenerateUniqueFileName(directory, "NewScript", ".cs");
			std::filesystem::path scriptPath = directory / filename;

			// Get class name from filename
			std::string className = std::filesystem::path(filename).stem().string();

			// Create the script file with template
			std::ofstream scriptFile(scriptPath);
			if (!scriptFile.is_open())
			{
				GX_CORE_ERROR("Failed to create script file: {0}", scriptPath.string());
				return false;
			}

			scriptFile << GetScriptTemplate(className);
			scriptFile.close();

			GX_CORE_INFO("Created new script: {0}", filename);
			return true;
		}
		catch (const std::exception& e)
		{
			GX_CORE_ERROR("Failed to create script: {0}", e.what());
			return false;
		}
	}

	bool AssetFactory::CreateGraphicsShader(const std::filesystem::path& directory)
	{
		try
		{
			// Ensure the directory exists
			if (!std::filesystem::exists(directory))
				std::filesystem::create_directories(directory);

			// Generate unique filename
			std::string filename = GenerateUniqueFileName(directory, "NewGraphicsShader", ".slang");
			std::filesystem::path shaderPath = directory / filename;

			// Create the shader file with template
			std::ofstream shaderFile(shaderPath);
			if (!shaderFile.is_open())
			{
				GX_CORE_ERROR("Failed to create graphics shader file: {0}", shaderPath.string());
				return false;
			}

			shaderFile << GetGraphicsShaderTemplate();
			shaderFile.close();

			// Import into asset manager
			auto assetDirectory = Project::GetAssetDirectory();
			auto relativePath = std::filesystem::relative(shaderPath, assetDirectory);
			auto assetManager = Project::GetActive()->GetEditorAssetManager();
			assetManager->ImportAsset(relativePath);
			assetManager->SerializeAssetRegistry();

			GX_CORE_INFO("Created new graphics shader: {0}", filename);
			return true;
		}
		catch (const std::exception& e)
		{
			GX_CORE_ERROR("Failed to create graphics shader: {0}", e.what());
			return false;
		}
	}

	bool AssetFactory::CreateComputeShader(const std::filesystem::path& directory)
	{
		try
		{
			// Ensure the directory exists
			if (!std::filesystem::exists(directory))
				std::filesystem::create_directories(directory);

			// Generate unique filename
			std::string filename = GenerateUniqueFileName(directory, "NewComputeShader", ".slang");
			std::filesystem::path shaderPath = directory / filename;

			// Create the shader file with template
			std::ofstream shaderFile(shaderPath);
			if (!shaderFile.is_open())
			{
				GX_CORE_ERROR("Failed to create compute shader file: {0}", shaderPath.string());
				return false;
			}

			shaderFile << GetComputeShaderTemplate();
			shaderFile.close();

			// Import into asset manager
			auto assetDirectory = Project::GetAssetDirectory();
			auto relativePath = std::filesystem::relative(shaderPath, assetDirectory);
			auto assetManager = Project::GetActive()->GetEditorAssetManager();
			assetManager->ImportAsset(relativePath);
			assetManager->SerializeAssetRegistry();

			GX_CORE_INFO("Created new compute shader: {0}", filename);
			return true;
		}
		catch (const std::exception& e)
		{
			GX_CORE_ERROR("Failed to create compute shader: {0}", e.what());
			return false;
		}
	}

	bool AssetFactory::CreatePipeline(const std::filesystem::path& directory)
	{
		try
		{
			// Ensure the directory exists
			if (!std::filesystem::exists(directory))
				std::filesystem::create_directories(directory);

			// Generate unique filename
			std::string filename = GenerateUniqueFileName(directory, "NewPipeline", ".pipeline");
			std::filesystem::path pipelinePath = directory / filename;

			// Create default pipeline configuration
			PipelineConfiguration defaultConfig{};
			defaultConfig.BlendingMode = Blending::Alpha;
			defaultConfig.EnableDepthTest = true;
			defaultConfig.EnableDepthWrite = true;
			defaultConfig.DepthCompareOp = CompareOp::Less;
			defaultConfig.CullMode = Cull::Back;
			defaultConfig.FrontFaceWinding = FrontFace::CounterClockwise;
			defaultConfig.FillMode = Fill::Solid;
			defaultConfig.GraphicsTopology = Topology::TriangleList;
			defaultConfig.LineWidth = 1.0f;

			Ref<Pipeline> pipeline = CreateRef<Pipeline>(defaultConfig);

			// Export pipeline to file
			PipelineImporter::ExportPipeline(pipelinePath, pipeline);

			// Import into asset manager
			auto assetDirectory = Project::GetAssetDirectory();
			auto relativePath = std::filesystem::relative(pipelinePath, assetDirectory);
			auto assetManager = Project::GetActive()->GetEditorAssetManager();
			assetManager->ImportAsset(relativePath);
			assetManager->SerializeAssetRegistry();

			GX_CORE_INFO("Created new pipeline: {0}", filename);
			return true;
		}
		catch (const std::exception& e)
		{
			GX_CORE_ERROR("Failed to create pipeline: {0}", e.what());
			return false;
		}
	}

	bool AssetFactory::CreateMaterial(const std::filesystem::path& directory)
	{
		try
		{
			// Ensure the directory exists
			if (!std::filesystem::exists(directory))
				std::filesystem::create_directories(directory);

			// Generate unique filename
			std::string filename = GenerateUniqueFileName(directory, "NewMaterial", ".orbmat");
			std::filesystem::path materialPath = directory / filename;

			// For now, create a material with null/default handles
			// Users will need to assign a shader and pipeline in the inspector
			AssetHandle nullHandle = 0;

			// Export material with placeholder handles
			MaterialImporter::ExportMaterial(materialPath, nullHandle, nullHandle);

			// Import into asset manager
			auto assetDirectory = Project::GetAssetDirectory();
			auto relativePath = std::filesystem::relative(materialPath, assetDirectory);
			auto assetManager = Project::GetActive()->GetEditorAssetManager();
			assetManager->ImportAsset(relativePath);
			assetManager->SerializeAssetRegistry();

			GX_CORE_INFO("Created new material: {0} (assign Shader and Pipeline in Inspector)", filename);
			return true;
		}
		catch (const std::exception& e)
		{
			GX_CORE_ERROR("Failed to create material: {0}", e.what());
			return false;
		}
	}

	bool AssetFactory::CreateFolder(const std::filesystem::path& parentDirectory, const std::string& folderName)
	{
		try
		{
			std::filesystem::path folderPath = parentDirectory / folderName;
			int counter = 1;

			while (std::filesystem::exists(folderPath))
			{
				folderPath = parentDirectory / (folderName + " " + std::to_string(counter++));
			}

			std::filesystem::create_directory(folderPath);
			GX_CORE_INFO("Created folder: {0}", folderPath.filename().string());
			return true;
		}
		catch (const std::filesystem::filesystem_error& e)
		{
			GX_CORE_ERROR("Failed to create folder: {0}", e.what());
			return false;
		}
	}

}
