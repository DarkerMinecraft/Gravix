#pragma once

#include <filesystem>
#include <string>

namespace Gravix
{

	class AssetFactory
	{
	public:
		// Create a new scene file
		static bool CreateScene(const std::filesystem::path& directory);

		// Create a new C# script file
		static bool CreateScript(const std::filesystem::path& directory);

		// Create a new graphics shader file (.slang)
		static bool CreateGraphicsShader(const std::filesystem::path& directory);

		// Create a new compute shader file (.slang)
		static bool CreateComputeShader(const std::filesystem::path& directory);

		// Create a new pipeline file (.pipeline)
		static bool CreatePipeline(const std::filesystem::path& directory);

		// Create a new material file (.orbmat)
		static bool CreateMaterial(const std::filesystem::path& directory);

		// Create a new folder
		static bool CreateFolder(const std::filesystem::path& parentDirectory, const std::string& folderName = "New Folder");

	private:
		static std::string GenerateUniqueFileName(const std::filesystem::path& directory, const std::string& baseName, const std::string& extension);
		static std::string GetScriptTemplate(const std::string& className);
		static std::string GetGraphicsShaderTemplate();
		static std::string GetComputeShaderTemplate();
	};

}
