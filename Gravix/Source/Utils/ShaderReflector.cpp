#include "pch.h"
#include "ShaderReflector.h"

#include "Utils/SlangTypeUtils.h"
#include "Core/Log.h"

#include <map>

using namespace slang;
namespace Gravix
{

	void ShaderReflector::ExtractPushConstants(slang::ProgramLayout* programLayout, ShaderReflection* reflection)
	{
		GX_CORE_INFO("--- Reflecting Push Constants ---");
		if (!programLayout) return;

		// Map to collect data for each unique push constant block.
		std::map<std::string, PushConstantRange> pushConstantRanges;

		// Step 1: Discover all global push constant blocks.
		slang::VariableLayoutReflection* globalScope = programLayout->getGlobalParamsVarLayout();
		if (globalScope)
		{
			slang::TypeLayoutReflection* globalType = globalScope->getTypeLayout();
			for (uint32_t i = 0; i < globalType->getFieldCount(); ++i)
			{
				slang::VariableLayoutReflection* globalVar = globalType->getFieldByIndex(i);
				if (globalVar->getCategory() == slang::ParameterCategory::PushConstantBuffer)
				{
					const char* blockName = globalVar->getName();
					if (pushConstantRanges.find(blockName) == pushConstantRanges.end())
					{
						PushConstantRange pcRange{};
						// THE FIX: Use our helper which calls your CalculateTypeSize.
						pcRange.Size = SlangTypeUtils::GetCorrectParameterSize(globalVar);
						pcRange.Offset = 0;
						pushConstantRanges[blockName] = pcRange;
					}
				}
			}
		}

		// Step 3: Finalize and store the reflection data.
		for (const auto& pair : pushConstantRanges)
		{
			reflection->AddPushConstantRange(pair.first, pair.second);
			GX_CORE_INFO("  Finalized Block: '{0}', Size: {1}",
				pair.first,
				pair.second.Size);
		}
	}

	void ShaderReflector::ExtractVertexAttributes(slang::EntryPointReflection* entryPointReflection, ShaderReflection* reflection)
	{
		GX_CORE_INFO("  Vertex Attributes:");

		uint32_t currentOffset = 0;
		uint32_t paramCount = entryPointReflection->getParameterCount();

		for (uint32_t paramIndex = 0; paramIndex < paramCount; paramIndex++)
		{
			auto param = entryPointReflection->getParameterByIndex(paramIndex);
			if (!param) continue;

			auto paramType = param->getType();
			if (!paramType) continue;

			// Check if this is a varying input (vertex shader input)
			if (param->getCategory() == slang::ParameterCategory::VaryingInput)
			{
				// Check if the parameter type is a struct (like VSInput in your triangle.slang)
				if (paramType->getKind() == slang::TypeReflection::Kind::Struct)
				{
					GX_CORE_INFO("    Found vertex input struct with {} fields", paramType->getFieldCount());

					uint32_t fieldCount = paramType->getFieldCount();
					for (uint32_t fieldIndex = 0; fieldIndex < fieldCount; fieldIndex++)
					{
						auto field = paramType->getFieldByIndex(fieldIndex);
						if (!field) continue;

						VertexAttribute attribute;

						// Get the field name (this is what you want: position, texCoords, normal, color)
						const char* fieldName = field->getName();
						attribute.Name = fieldName ? fieldName : ("field_" + std::to_string(fieldIndex));

						// Generate semantic from field name
						attribute.Semantic = SlangTypeUtils::GenerateSemanticFromName(attribute.Name);

						// Use field index as location
						attribute.Location = fieldIndex;

						// Get field type and convert it
						auto fieldType = field->getType();
						attribute.Type = SlangTypeUtils::SlangTypeToShaderDataType(fieldType);

						if (attribute.Type == ShaderDataType::None)
						{
							GX_CORE_WARN("Skipping field '{}' with unknown type", attribute.Name);
							continue;
						}

						attribute.Size = ShaderDataTypeSize(attribute.Type);
						attribute.Offset = currentOffset;
						attribute.Normalized = false;

						// Special handling for color attributes
						if (attribute.Semantic == "COLOR" || attribute.Semantic.find("COLOR") != std::string::npos)
						{
							attribute.Normalized = true;
						}

						currentOffset += attribute.Size;

						// Add to reflection data (no entryPointIndex needed)
						reflection->AddVertexAttribute(attribute);

						GX_CORE_INFO("      Name: {0}", attribute.Name);
						GX_CORE_INFO("      Semantic: {0}", attribute.Semantic);
						GX_CORE_INFO("      Location: {0}", attribute.Location);
						GX_CORE_INFO("      Type: {0}", SlangTypeUtils::ShaderDataTypeToString(attribute.Type));
						GX_CORE_INFO("      Size: {0} bytes", attribute.Size);
						GX_CORE_INFO("      Offset: {0}", attribute.Offset);
						GX_CORE_INFO("      Normalized: {0}", attribute.Normalized ? "true" : "false");
					}
				}
				else
				{
					// Handle non-struct inputs (single parameters)
					VertexAttribute attribute;

					const char* paramName = param->getVariable() ? param->getVariable()->getName() : nullptr;
					attribute.Name = paramName ? paramName : ("param_" + std::to_string(paramIndex));
					attribute.Semantic = SlangTypeUtils::GenerateSemanticFromName(attribute.Name);
					attribute.Location = paramIndex;
					attribute.Type = SlangTypeUtils::SlangTypeToShaderDataType(paramType);

					if (attribute.Type != ShaderDataType::None)
					{
						attribute.Size = ShaderDataTypeSize(attribute.Type);
						attribute.Offset = currentOffset;
						attribute.Normalized = (attribute.Semantic.find("COLOR") != std::string::npos);

						currentOffset += attribute.Size;
						reflection->AddVertexAttribute(attribute);

						GX_CORE_INFO("      Name: {0}", attribute.Name);
						GX_CORE_INFO("      Semantic: {0}", attribute.Semantic);
						GX_CORE_INFO("      Location: {0}", attribute.Location);
						GX_CORE_INFO("      Type: {0}", SlangTypeUtils::ShaderDataTypeToString(attribute.Type));
						GX_CORE_INFO("      Size: {0} bytes", attribute.Size);
						GX_CORE_INFO("      Offset: {0}", attribute.Offset);
						GX_CORE_INFO("      Normalized: {0}", attribute.Normalized ? "true" : "false");
					}
				}
			}
		}

		reflection->SetVertexStride(currentOffset);
		GX_CORE_INFO("  Total Vertex Stride: {0} bytes", currentOffset);
	}

	void ShaderReflector::ExtractComputeDispatchInfo(slang::EntryPointReflection* entryPoint, ShaderReflection* reflection)
	{
		ComputeDispatchInfo dispatchInfo{};

		SlangUInt threadGroupSize[3];
		entryPoint->getComputeThreadGroupSize(3, threadGroupSize);

		dispatchInfo.LocalSizeX = threadGroupSize[0];
		dispatchInfo.LocalSizeY = threadGroupSize[1];
		dispatchInfo.LocalSizeZ = threadGroupSize[2];

		reflection->AddDispatchGroups(dispatchInfo);
		GX_CORE_INFO("  Compute Thread Group Size: ({0}, {1}, {2})", dispatchInfo.LocalSizeX, dispatchInfo.LocalSizeY, dispatchInfo.LocalSizeZ);
	}

	void ShaderReflector::ExtractStructs(slang::ProgramLayout* layout, ShaderReflection* reflection)
	{
		GX_CORE_INFO("--- Reflecting Structs ---");
		if (!layout) return;

		auto globalParamsVarLayout = layout->getGlobalParamsVarLayout();
		if (!globalParamsVarLayout)
		{
			GX_CORE_WARN("  No global params var layout found.");
			return;
		}

		// Normalize to the container var layout if parameter block / cb / ssbo is used
		{
			auto tLayout = globalParamsVarLayout->getTypeLayout();
			if (tLayout->getKind() == slang::TypeReflection::Kind::ParameterBlock)
			{
				globalParamsVarLayout = tLayout->getElementVarLayout();
				tLayout = globalParamsVarLayout->getTypeLayout();
			}
			if (tLayout->getKind() == slang::TypeReflection::Kind::ConstantBuffer)
			{
				globalParamsVarLayout = tLayout->getContainerVarLayout();
			}
			if (tLayout->getKind() == slang::TypeReflection::Kind::ShaderStorageBuffer)
			{
				globalParamsVarLayout = tLayout->getContainerVarLayout();
			}
		}

		auto structLayout = globalParamsVarLayout->getTypeLayout();
		if (!structLayout)
		{
			GX_CORE_WARN("  Global params type layout missing.");
			return;
		}

		std::set<std::string> processedStructs;

		// Process each field to find structs
		for (uint32_t i = 0; i < structLayout->getFieldCount(); ++i)
		{
			auto field = structLayout->getFieldByIndex(i);
			if (!field) continue;

			auto fieldTypeLayout = field->getTypeLayout();
			if (!fieldTypeLayout) continue;

			auto fieldType = fieldTypeLayout->getType();
			if (!fieldType) continue;

			ProcessStructType(fieldTypeLayout, fieldType, processedStructs, reflection);
		}
	}

	void ShaderReflector::ExtractStructsFromPointers(slang::ProgramLayout* layout, ShaderReflection* reflection)
	{
		GX_CORE_INFO("--- Reflecting Structs from Pointers ---");
		if (!layout) return;

		std::set<std::string> processedStructs;

		// Iterate through all entry points to find push constant pointers
		uint32_t entryPointCount = layout->getEntryPointCount();
		for (uint32_t entryIdx = 0; entryIdx < entryPointCount; entryIdx++)
		{
			auto entryPoint = layout->getEntryPointByIndex(entryIdx);
			if (!entryPoint) continue;

			GX_CORE_TRACE("  Scanning entry point: {0}", entryPoint->getName());

			auto entryPointVarLayout = entryPoint->getVarLayout();
			if (!entryPointVarLayout) continue;

			auto scopeTypeLayout = entryPointVarLayout->getTypeLayout();
			if (!scopeTypeLayout) continue;

			// Handle potential constant buffer/parameter block wrapping
			if (scopeTypeLayout->getKind() == slang::TypeReflection::Kind::ConstantBuffer ||
				scopeTypeLayout->getKind() == slang::TypeReflection::Kind::ParameterBlock)
			{
				scopeTypeLayout = scopeTypeLayout->getElementTypeLayout();
			}

			if (!scopeTypeLayout) continue;

			// Iterate through all fields in the entry point scope
			uint32_t fieldCount = scopeTypeLayout->getFieldCount();
			for (uint32_t fieldIdx = 0; fieldIdx < fieldCount; fieldIdx++)
			{
				auto paramVarLayout = scopeTypeLayout->getFieldByIndex(fieldIdx);
				if (!paramVarLayout) continue;

				// Check if this field is in a push constant
				if (paramVarLayout->getCategory() != slang::ParameterCategory::PushConstantBuffer)
					continue;

				const char* fieldName = paramVarLayout->getName();
				GX_CORE_TRACE("    Found push constant field: {0}", fieldName ? fieldName : "<unnamed>");

				auto typeLayout = paramVarLayout->getTypeLayout();
				if (!typeLayout) continue;

				auto type = typeLayout->getType();
				if (!type) continue;

				// Process this field to find pointer types
				ProcessPointerType(typeLayout, type, processedStructs, reflection);
			}
		}

		// Also check global scope push constants
		slang::VariableLayoutReflection* globalScope = layout->getGlobalParamsVarLayout();
		if (globalScope)
		{
			slang::TypeLayoutReflection* globalType = globalScope->getTypeLayout();
			if (globalType)
			{
				for (uint32_t i = 0; i < globalType->getFieldCount(); ++i)
				{
					slang::VariableLayoutReflection* globalVar = globalType->getFieldByIndex(i);
					if (!globalVar) continue;

					if (globalVar->getCategory() == slang::ParameterCategory::PushConstantBuffer)
					{
						const char* fieldName = globalVar->getName();
						GX_CORE_TRACE("  Found global push constant: {0}", fieldName ? fieldName : "<unnamed>");

						auto typeLayout = globalVar->getTypeLayout();
						if (!typeLayout) continue;

						auto type = typeLayout->getType();
						if (!type) continue;

						ProcessPointerType(typeLayout, type, processedStructs, reflection);
					}
				}
			}
		}
	}

	void ShaderReflector::ProcessPointerType(slang::TypeLayoutReflection* typeLayout,
		slang::TypeReflection* type,
		std::set<std::string>& processedStructs,
		ShaderReflection* reflection)
	{
		if (!type || !typeLayout) return;

		auto typeKind = type->getKind();

		// Handle pointer types (kind 18)
		if (typeKind == static_cast<slang::TypeReflection::Kind>(18))
		{
			GX_CORE_TRACE("    Found pointer type");

			// Get the element type (what the pointer points to)
			auto elementTypeLayout = typeLayout->getElementTypeLayout();
			if (!elementTypeLayout)
			{
				GX_CORE_WARN("    Pointer has no element type layout");
				return;
			}

			auto elementType = elementTypeLayout->getType();
			if (!elementType)
			{
				GX_CORE_WARN("    Pointer has no element type");
				return;
			}

			// Check if the element is a struct
			if (elementType->getKind() == slang::TypeReflection::Kind::Struct)
			{
				const char* structName = elementType->getName();
				if (!structName)
				{
					GX_CORE_WARN("    Pointed-to struct has no name");
					return;
				}

				std::string structNameStr(structName);

				// Skip if already processed
				if (processedStructs.find(structNameStr) != processedStructs.end())
				{
					GX_CORE_TRACE("    Struct '{0}' already processed", structNameStr);
					return;
				}

				processedStructs.insert(structNameStr);

				GX_CORE_INFO("  Found Struct from Pointer: '{0}'", structNameStr);

				// Extract the struct information
				ReflectedStruct reflectedStruct;
				reflectedStruct.Name = structNameStr;

				uint32_t fieldCount = elementTypeLayout->getFieldCount();
				size_t calculatedSize = 0;

				for (uint32_t i = 0; i < fieldCount; ++i)
				{
					slang::VariableLayoutReflection* field = elementTypeLayout->getFieldByIndex(i);
					if (!field) continue;

					const char* fieldName = field->getName();
					if (!fieldName) continue;

					slang::TypeLayoutReflection* fieldTypeLayout = field->getTypeLayout();
					if (!fieldTypeLayout) continue;

					auto fieldType = fieldTypeLayout->getType();
					if (!fieldType) continue;

					// Calculate tightly-packed offset (no padding)
					size_t fieldSize = SlangTypeUtils::CalculateTypeSize(fieldType);

					ReflectedStructMember member;
					member.Name = fieldName;
					member.Offset = calculatedSize;
					member.Size = fieldSize;

					reflectedStruct.Members.push_back(member);

					GX_CORE_TRACE("    Member: '{0}', Offset: {1}, Size: {2}",
						member.Name, member.Offset, member.Size);

					calculatedSize += fieldSize;
				}

				// Use tightly-packed size
				reflectedStruct.Size = calculatedSize;

				GX_CORE_INFO("  Extracted Struct from Pointer: '{0}', Size: {1} (tightly packed), Members: {2}",
					reflectedStruct.Name, reflectedStruct.Size, reflectedStruct.Members.size());

				reflection->AddReflectedStruct(structNameStr, reflectedStruct);
			}
			else
			{
				// The pointer doesn't point to a struct, but might contain nested pointers
				// Recursively process it
				ProcessPointerType(elementTypeLayout, elementType, processedStructs, reflection);
			}
		}
		// Handle structs that might contain pointer fields
		else if (typeKind == slang::TypeReflection::Kind::Struct)
		{
			uint32_t fieldCount = typeLayout->getFieldCount();
			for (uint32_t i = 0; i < fieldCount; ++i)
			{
				auto field = typeLayout->getFieldByIndex(i);
				if (!field) continue;

				auto fieldTypeLayout = field->getTypeLayout();
				if (!fieldTypeLayout) continue;

				auto fieldType = fieldTypeLayout->getType();
				if (!fieldType) continue;

				// Recursively check struct fields for pointers
				ProcessPointerType(fieldTypeLayout, fieldType, processedStructs, reflection);
			}
		}
		// Handle constant buffers - these might wrap structs we want to extract
		else if (typeKind == slang::TypeReflection::Kind::ConstantBuffer ||
			typeKind == slang::TypeReflection::Kind::ShaderStorageBuffer)
		{
			auto elementTypeLayout = typeLayout->getElementTypeLayout();
			if (elementTypeLayout)
			{
				auto elementType = elementTypeLayout->getType();
				if (elementType)
				{
					ProcessPointerType(elementTypeLayout, elementType, processedStructs, reflection);
				}
			}
		}
	}

	void ShaderReflector::ProcessStructType(slang::TypeLayoutReflection* typeLayout,
		slang::TypeReflection* type,
		std::set<std::string>& processedStructs,
		ShaderReflection* reflection)
	{
		if (!type || !typeLayout) return;

		auto typeKind = type->getKind();

		// If it's a struct, extract it
		if (typeKind == slang::TypeReflection::Kind::Struct)
		{
			const char* structName = type->getName();
			if (!structName) return;

			std::string structNameStr(structName);

			// Skip VS/PS input/output structs
			if (structNameStr.find("VS") == 0 || structNameStr.find("PS") == 0)
			{
				GX_CORE_TRACE("  Skipping shader stage struct: '{0}'", structName);
				return;
			}

			// Skip if already processed
			if (processedStructs.find(structName) != processedStructs.end())
				return;

			processedStructs.insert(structName);

			GX_CORE_INFO("  Found Struct: '{0}'", structName);

			// Extract struct inline
			ReflectedStruct reflectedStruct;
			reflectedStruct.Name = structName;

			uint32_t fieldCount = typeLayout->getFieldCount();
			size_t calculatedSize = 0;

			for (uint32_t i = 0; i < fieldCount; ++i)
			{
				slang::VariableLayoutReflection* field = typeLayout->getFieldByIndex(i);
				if (!field) continue;

				const char* fieldName = field->getName();
				if (!fieldName) continue;

				slang::TypeLayoutReflection* fieldTypeLayout = field->getTypeLayout();
				if (!fieldTypeLayout) continue;

				auto fieldType = fieldTypeLayout->getType();
				if (!fieldType) continue;

				// Calculate tightly-packed offset (no padding)
				size_t fieldSize = SlangTypeUtils::CalculateTypeSize(fieldType);

				ReflectedStructMember member;
				member.Name = fieldName;
				member.Offset = calculatedSize;  // Use calculated offset, not Slang's padded offset
				member.Size = fieldSize;

				reflectedStruct.Members.push_back(member);

				GX_CORE_TRACE("    Member: '{0}', Offset: {1}, Size: {2} (Slang offset: {3})",
					member.Name, member.Offset, member.Size, field->getOffset());

				calculatedSize += fieldSize;
			}

			// Use tightly-packed size for vertex buffers, not Slang's padded size
			reflectedStruct.Size = calculatedSize;

			GX_CORE_INFO("  Extracted Struct: '{0}', Size: {1} (tightly packed), Slang Size: {2} (padded), Members: {3}",
				reflectedStruct.Name, reflectedStruct.Size, typeLayout->getSize(), reflectedStruct.Members.size());

			reflection->AddReflectedStruct(structNameStr, reflectedStruct);

			// Recursively process nested structs
			for (uint32_t i = 0; i < typeLayout->getFieldCount(); ++i)
			{
				auto field = typeLayout->getFieldByIndex(i);
				if (!field) continue;

				auto fieldTypeLayout = field->getTypeLayout();
				if (!fieldTypeLayout) continue;

				auto fieldType = fieldTypeLayout->getType();
				if (!fieldType) continue;

				ProcessStructType(fieldTypeLayout, fieldType, processedStructs, reflection);
			}
		}
		// Handle arrays of structs
		else if (typeKind == slang::TypeReflection::Kind::Array)
		{
			auto elementType = type->getElementType();
			if (elementType && elementType->getKind() == slang::TypeReflection::Kind::Struct)
			{
				auto elementTypeLayout = typeLayout->getElementTypeLayout();
				ProcessStructType(elementTypeLayout, elementType, processedStructs, reflection);
			}
		}
		// Handle constant buffers and storage buffers containing structs
		else if (typeKind == slang::TypeReflection::Kind::ConstantBuffer ||
			typeKind == slang::TypeReflection::Kind::ShaderStorageBuffer)
		{
			auto elementTypeLayout = typeLayout->getElementTypeLayout();
			if (elementTypeLayout)
			{
				auto elementType = elementTypeLayout->getType();
				if (elementType)
				{
					ProcessStructType(elementTypeLayout, elementType, processedStructs, reflection);
				}
			}
		}
	}

}
