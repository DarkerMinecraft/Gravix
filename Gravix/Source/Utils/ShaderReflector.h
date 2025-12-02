#pragma once

#include "Reflections/ShaderReflection.h"

#include <slang.h>
#include <string>
#include <set>

namespace Gravix
{

	class ShaderReflector
	{
	public:
		// Main extraction methods
		static void ExtractPushConstants(slang::ProgramLayout* programLayout, ShaderReflection* reflection);
		static void ExtractVertexAttributes(slang::EntryPointReflection* entryPointReflection, ShaderReflection* reflection);
		static void ExtractComputeDispatchInfo(slang::EntryPointReflection* entryPoint, ShaderReflection* reflection);
		static void ExtractStructs(slang::ProgramLayout* layout, ShaderReflection* reflection);
		static void ExtractStructsFromPointers(slang::ProgramLayout* layout, ShaderReflection* reflection);

	private:
		// Helper methods for struct processing
		static void ProcessPointerType(slang::TypeLayoutReflection* typeLayout,
			slang::TypeReflection* type,
			std::set<std::string>& processedStructs,
			ShaderReflection* reflection);

		static void ProcessStructType(slang::TypeLayoutReflection* typeLayout,
			slang::TypeReflection* type,
			std::set<std::string>& processedStructs,
			ShaderReflection* reflection);
	};

}
