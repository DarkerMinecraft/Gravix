#pragma once

#include "ShaderCompiler.h"
#include "Core/RefCounted.h"

#include <memory>

namespace Gravix
{

	// Global ShaderCompiler system for the editor
	// Removed in runtime builds
	class ShaderCompilerSystem
	{
	public:
		static void Initialize();
		static void Shutdown();

		static ShaderCompiler& Get();
	private:
		static std::unique_ptr<ShaderCompiler> s_Compiler;
	};

}
