#pragma once

#include <filesystem>

namespace Gravix 
{
	
	class Project 
	{
	public:
		Project(const std::filesystem::path& workingDirectory) 
			: m_WorkingDirectory(workingDirectory), 
			  m_AssetsDirectory(workingDirectory / "Assets"), 
			  m_ProjectLibraryDirectory(workingDirectory / "Library") {}
		Project() = default;

		void SetWorkingDirectory(const std::filesystem::path& path) 
		{
			m_WorkingDirectory = path;
			m_AssetsDirectory = m_WorkingDirectory / "Assets";
			m_ProjectLibraryDirectory = m_WorkingDirectory / "Library";
		}

		void CreateProjectDirectories() const 
		{
			if(!std::filesystem::exists(m_WorkingDirectory))
				std::filesystem::create_directories(m_WorkingDirectory);
			if(!std::filesystem::exists(m_AssetsDirectory))
				std::filesystem::create_directories(m_AssetsDirectory);
			if (!std::filesystem::exists(m_ProjectLibraryDirectory))
				std::filesystem::create_directories(m_ProjectLibraryDirectory);
		}

		const std::filesystem::path& GetWorkingDirectory() const { return m_WorkingDirectory; }
		const std::filesystem::path& GetAssetsDirectory() const { return m_AssetsDirectory; }
		const std::filesystem::path& GetProjectLibraryDirectory() const { return m_ProjectLibraryDirectory; }
	private:
		std::filesystem::path m_WorkingDirectory;
		std::filesystem::path m_AssetsDirectory;
		std::filesystem::path m_ProjectLibraryDirectory;
	};

}