#pragma once

#include "Renderer/Specification.h"
#include "Reflections/ShaderReflection.h"

#include <slang.h>
#include <slang-com-ptr.h>

#include <string>
#include <vector>
#include <filesystem>

namespace Gravix
{

	class ShaderCompiler
	{
	public:
		ShaderCompiler();

		bool CompileShader(const std::filesystem::path& filePath, std::vector<std::vector<uint32_t>>* spirvCode, ShaderReflection* reflection);

	private:
		Slang::ComPtr<slang::IGlobalSession> m_GlobalSession;
	};

}
