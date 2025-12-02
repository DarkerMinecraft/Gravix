#include "pch.h"
#include "ShaderCompilerSystem.h"

namespace Gravix
{

	std::unique_ptr<ShaderCompiler> ShaderCompilerSystem::s_Compiler = nullptr;

	void ShaderCompilerSystem::Initialize()
	{
		if (!s_Compiler)
		{
			s_Compiler = std::make_unique<ShaderCompiler>();
			GX_CORE_INFO("ShaderCompiler system initialized");
		}
	}

	void ShaderCompilerSystem::Shutdown()
	{
		if (s_Compiler)
		{
			s_Compiler.reset();
			GX_CORE_INFO("ShaderCompiler system shutdown");
		}
	}

	ShaderCompiler& ShaderCompilerSystem::Get()
	{
		GX_ASSERT(s_Compiler, "ShaderCompiler system not initialized!");
		return *s_Compiler;
	}

}
