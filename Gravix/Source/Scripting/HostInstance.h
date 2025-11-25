#pragma once

#include <filesystem>

namespace Gravix 
{

	class HostInstance 
	{
	public:
		HostInstance(const std::filesystem::path& assemblyPath); 
	private:
		void Initialize(const std::filesystem::path& filePath);
	};

}