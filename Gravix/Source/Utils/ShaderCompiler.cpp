#include "pch.h"
#include "ShaderCompiler.h"

#include "Reflections/ShaderReflection.h"
#include "Renderer/Vulkan/VulkanRenderCaps.h"

#include <filesystem>
#include <algorithm>
#include <cstring>

using namespace slang;
namespace Gravix
{

	static const char* ShaderStageToString(ShaderStage stage)
	{
		switch (stage)
		{
		case ShaderStage::Vertex:   return "Vertex";
		case ShaderStage::Fragment: return "Fragment";
		case ShaderStage::Compute:  return "Compute";
		case ShaderStage::Geometry: return "Geometry";
		case ShaderStage::None:     return "None";
		case ShaderStage::All:      return "All";
		default:                    return "Unknown";
		}
	}

	static const char* DescriptorTypeToString(DescriptorType type)
	{
		switch (type)
		{
		case DescriptorType::UniformBuffer: return "UniformBuffer";
		case DescriptorType::StorageBuffer: return "StorageBuffer";
		case DescriptorType::SampledImage:  return "SampledImage";
		case DescriptorType::StorageImage:  return "StorageImage";
		default:                            return "Unknown";
		}
	}

	ShaderCompiler::ShaderCompiler()
	{
		createGlobalSession(m_GlobalSession.writeRef());
	}

	bool ShaderCompiler::CompileShader(const std::filesystem::path& filePath, std::vector<std::vector<uint32_t>>* spirvCodes, ShaderReflection* reflection)
	{
		GX_CORE_INFO("Compiling shader: {0}", filePath.string());

		SessionDesc sessionDesc{};
		TargetDesc targetDesc{};
		targetDesc.format = SLANG_SPIRV;
		targetDesc.profile = m_GlobalSession->findProfile("spirv_1_5");

		sessionDesc.targets = &targetDesc;
		sessionDesc.targetCount = 1;

		std::vector<CompilerOptionEntry> options;
		options.push_back({
			CompilerOptionName::VulkanUseEntryPointName,
			{CompilerOptionValueKind::Int, 1, 0, nullptr, nullptr}
			});
		options.push_back({
			CompilerOptionName::Optimization,
			{CompilerOptionValueKind::Int, 1, 0, nullptr, nullptr}
			});
		options.push_back({
			CompilerOptionName::GLSLForceScalarLayout,
			{CompilerOptionValueKind::Int, 1, 0, nullptr, nullptr}
			});
		sessionDesc.compilerOptionEntries = options.data();
		sessionDesc.compilerOptionEntryCount = static_cast<uint32_t>(options.size());

		Slang::ComPtr<ISession> session;
		m_GlobalSession->createSession(sessionDesc, session.writeRef());

		Slang::ComPtr<IModule> slangModule;
		{
			Slang::ComPtr<IBlob> diagnosticBlob;
			slangModule = session->loadModule(filePath.string().c_str(), diagnosticBlob.writeRef());
			if (diagnosticBlob && diagnosticBlob->getBufferSize() > 0)
			{
				std::string message = (char*)diagnosticBlob->getBufferPointer();
				GX_CORE_CRITICAL("Failed to load shader: {0}", message);
				return false;
			}
		}

		if (!slangModule) return false;

		// --- Explicitly Collect Entry Points for Linking ---
		std::vector<IComponentType*> componentTypes;
		componentTypes.push_back(slangModule); // Start with the module itself

		uint32_t definedEntryPointCount = slangModule->getDefinedEntryPointCount();
		Slang::ComPtr<IEntryPoint> entryPoints[32]; // Max 32 entry points, adjust if needed
		for (uint32_t i = 0; i < definedEntryPointCount; i++)
		{
			slangModule->getDefinedEntryPoint(i, entryPoints[i].writeRef());
			componentTypes.push_back(entryPoints[i]);
		}

		// --- Create a Composite Program and Link It ---
		Slang::ComPtr<slang::IComponentType> composedProgram;
		{
			Slang::ComPtr<IBlob> diagnosticsBlob;
			SlangResult result = session->createCompositeComponentType(
				componentTypes.data(),
				componentTypes.size(),
				composedProgram.writeRef(),
				diagnosticsBlob.writeRef());

			if (diagnosticsBlob && diagnosticsBlob->getBufferSize() > 0)
			{
				// Handle diagnostics...
				if (SLANG_FAILED(result)) return false;
			}
		}

		Slang::ComPtr<slang::IComponentType> linkedProgram;
		{
			Slang::ComPtr<IBlob> diagnosticsBlob;
			SlangResult result = composedProgram->link(linkedProgram.writeRef(), diagnosticsBlob.writeRef());

			if (diagnosticsBlob && diagnosticsBlob->getBufferSize() > 0)
			{
				// Handle diagnostics...
				if (SLANG_FAILED(result)) return false;
			}
		}

		if (!linkedProgram) return false;

		// --- Loop through the linked program's entry points ---
		slang::ProgramLayout* layout = linkedProgram->getLayout();
		ExtractPushConstants(layout, reflection);

		ExtractBuffers(layout, reflection);
		ExtractStructs(layout, reflection);

		uint32_t entryPointCount = layout->getEntryPointCount();

		for (uint32_t i = 0; i < entryPointCount; i++)
		{
			// 2. Get the reflection for the entry point at index 'i'.
			slang::EntryPointReflection* entryPointReflection = layout->getEntryPointByIndex(i);
			if (!entryPointReflection) continue;

			ShaderStage shaderStage = SlangStageToShaderStage(entryPointReflection->getStage());
			GX_CORE_INFO("Processing Entry Point: {0} ({1})", entryPointReflection->getName(), ShaderStageToString(shaderStage));

			Slang::ComPtr<slang::IBlob> spirvCode;
			{
				Slang::ComPtr<slang::IBlob> diagnosticsBlob;
				// 3. Get the code for the entry point at index 'i'.
				SlangResult result = linkedProgram->getEntryPointCode(
					i, // Use the loop index 'i' here
					0,
					spirvCode.writeRef(),
					diagnosticsBlob.writeRef());

				if (diagnosticsBlob && diagnosticsBlob->getBufferSize() > 0)
				{
					std::string message = (char*)diagnosticsBlob->getBufferPointer();
					GX_CORE_CRITICAL("Failed to get entry point code: {0}", message);
					if (SLANG_FAILED(result)) continue;
				}
			}

			if (!spirvCode || spirvCode->getBufferSize() == 0) continue;

			std::vector<uint32_t> spirv(spirvCode->getBufferSize() / sizeof(uint32_t));
			std::memcpy(spirv.data(), spirvCode->getBufferPointer(), spirvCode->getBufferSize());

			if (spirv.empty() || spirv[0] != 0x07230203) // SPIR-V Magic Number
			{
				GX_CORE_ERROR("Invalid SPIR-V magic number for entry point {0}", entryPointReflection->getName());
				continue;
			}

			spirvCodes->push_back(spirv);
			reflection->SetShaderName(filePath.stem().string());
			reflection->AddEntryPoint({ std::string(entryPointReflection->getName()), shaderStage });
			if (shaderStage == ShaderStage::Compute)
				ExtractComputeDispatchInfo(entryPointReflection, reflection);
			else if (shaderStage == ShaderStage::Vertex)
				ExtractVertexAttributes(entryPointReflection, reflection);
		}

		return true;
	}

	void ShaderCompiler::ExtractPushConstants(slang::ProgramLayout* programLayout, ShaderReflection* reflection)
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
						pcRange.Size = GetCorrectParameterSize(globalVar);
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

	void ShaderCompiler::ExtractVertexAttributes(slang::EntryPointReflection* entryPointReflection, ShaderReflection* reflection)
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
						attribute.Semantic = GenerateSemanticFromName(attribute.Name);

						// Use field index as location
						attribute.Location = fieldIndex;

						// Get field type and convert it
						auto fieldType = field->getType();
						attribute.Type = SlangTypeToShaderDataType(fieldType);

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
						GX_CORE_INFO("      Type: {0}", ShaderDataTypeToString(attribute.Type));
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
					attribute.Semantic = GenerateSemanticFromName(attribute.Name);
					attribute.Location = paramIndex;
					attribute.Type = SlangTypeToShaderDataType(paramType);

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
						GX_CORE_INFO("      Type: {0}", ShaderDataTypeToString(attribute.Type));
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

	void ShaderCompiler::ExtractComputeDispatchInfo(slang::EntryPointReflection* entryPoint, ShaderReflection* reflection)
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

	void ShaderCompiler::ExtractBuffers(slang::ProgramLayout* layout, ShaderReflection* reflection)
	{
		GX_CORE_INFO("--- Reflecting Resource Buffers ---");
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

		// Build a map of which resources are used by which stages
		std::map<std::string, ShaderStage> resourceStageMap;

		uint32_t entryPointCount = layout->getEntryPointCount();
		for (uint32_t ep = 0; ep < entryPointCount; ep++)
		{
			slang::EntryPointReflection* entryPoint = layout->getEntryPointByIndex(ep);
			ShaderStage entryPointStage = SlangStageToShaderStage(entryPoint->getStage());

			// Get the entry point's variable layout to check resource usage
			slang::VariableLayoutReflection* entryVarLayout = entryPoint->getVarLayout();
			if (!entryVarLayout) continue;

			// Check each global resource to see if this entry point uses it
			for (uint32_t i = 0; i < structLayout->getFieldCount(); ++i)
			{
				auto field = structLayout->getFieldByIndex(i);
				if (!field) continue;

				const char* name = field->getVariable() ? field->getVariable()->getName() : nullptr;
				if (!name) continue;

				// Check if the binding index is valid (indicating usage)
				uint32_t bindingInEntryPoint = field->getBindingIndex();

				// If binding is valid, this entry point uses this resource
				if (bindingInEntryPoint != UINT32_MAX)
				{
					auto it = resourceStageMap.find(name);
					if (it == resourceStageMap.end())
					{
						resourceStageMap[name] = entryPointStage;
					}
					else
					{
						// Resource used by multiple stages - OR them together
						resourceStageMap[name] = (ShaderStage)((uint32_t)it->second | (uint32_t)entryPointStage);
					}
				}
			}
		}

		// Now extract resources with the correct stage information
		for (uint32_t i = 0; i < structLayout->getFieldCount(); ++i)
		{
			auto field = structLayout->getFieldByIndex(i);
			if (!field) continue;

			const char* name = field->getVariable() ? field->getVariable()->getName() : nullptr;

			// Skip dummy reflection bindings FIRST
			if (name && name[0] == '_')
			{
				GX_CORE_TRACE("  Skipping dummy reflection binding: '{0}'", name);
				continue;
			}

			// Skip if already registered as push constant
			if (name && reflection->HasPushConstantRange(name))
			{
				GX_CORE_TRACE("  Skipping '{0}' - already registered as push constant", name);
				continue;
			}

			// Check if resource is used by any entry point
			if (name && resourceStageMap.find(name) == resourceStageMap.end())
			{
				GX_CORE_TRACE("  Resource '{0}' is declared but unused by any entry point", name ? name : "UNNAMED");
				continue;
			}

			uint32_t binding = field->getBindingIndex();
			uint32_t set = field->getBindingSpace();

			auto varTypeLayout = field->getTypeLayout();
			if (!varTypeLayout)
			{
				GX_CORE_WARN("  Field '{0}' has no type layout, skipping.", name ? name : "UNNAMED");
				continue;
			}

			auto type = varTypeLayout->getType();
			if (!type)
			{
				GX_CORE_WARN("  Field '{0}' has no type reflection, skipping.", name ? name : "UNNAMED");
				continue;
			}

			auto typeKind = type->getKind();
			auto resourceShape = type->getResourceShape();
			auto resourceAccess = type->getResourceAccess();

			// Skip invalid bindings
			if (binding == UINT32_MAX || set == UINT32_MAX)
			{
				GX_CORE_TRACE("  Resource '{0}' has invalid binding ({1}) or set ({2}) - skipping.", name ? name : "UNNAMED", binding, set);
				continue;
			}

			// Setup resource binding with detected stage
			ShaderResourceBinding rbo;
			rbo.Name = name ? name : "UNNAMED";
			rbo.Binding = binding;
			rbo.Set = set;
			rbo.Stage = name ? resourceStageMap[name] : ShaderStage::None;
			rbo.Count = 1;
			rbo.Type = DescriptorType::SampledImage; // default

			// Determine descriptor type
			if (typeKind == slang::TypeReflection::Kind::ConstantBuffer)
			{
				rbo.Type = DescriptorType::UniformBuffer;
				GX_CORE_TRACE("  Found ConstantBuffer: '{0}'", name);
			}
			else if (typeKind == slang::TypeReflection::Kind::ShaderStorageBuffer)
			{
				rbo.Type = DescriptorType::StorageBuffer;
				GX_CORE_TRACE("  Found ShaderStorageBuffer: '{0}'", name);
			}
			else if (typeKind == slang::TypeReflection::Kind::Resource)
			{
				SlangResourceShape baseShape = (SlangResourceShape)(resourceShape & SLANG_RESOURCE_BASE_SHAPE_MASK);

				switch (baseShape)
				{
				case SLANG_TEXTURE_1D:
				case SLANG_TEXTURE_2D:
				case SLANG_TEXTURE_3D:
				case SLANG_TEXTURE_CUBE:
				case SLANG_TEXTURE_BUFFER:
					if (resourceAccess == SLANG_RESOURCE_ACCESS_READ_WRITE)
					{
						rbo.Type = DescriptorType::StorageImage;
						GX_CORE_TRACE("  Found StorageImage: '{0}'", name);
					}
					else
					{
						rbo.Type = DescriptorType::SampledImage;
						GX_CORE_TRACE("  Found SampledImage: '{0}'", name);
					}
					break;

				case SLANG_STRUCTURED_BUFFER:
				case SLANG_BYTE_ADDRESS_BUFFER:
					rbo.Type = DescriptorType::StorageBuffer;
					GX_CORE_TRACE("  Found StructuredBuffer/ByteAddressBuffer: '{0}'", name);
					break;

				default:
					GX_CORE_TRACE("  Unknown resource shape {0} for '{1}', defaulting to SampledImage", (int)baseShape, name);
					break;
				}
			}

			// Handle arrays
			if (type->isArray())
			{
				SlangUInt arrayCount = type->getTotalArrayElementCount();
				if (arrayCount == 0)
				{
					// Unbounded array - use bindless count for images
					if (rbo.Type == DescriptorType::SampledImage || rbo.Type == DescriptorType::StorageImage)
					{
						rbo.Count = VulkanRenderCaps::GetRecommendedBindlessSampledImages();
						GX_CORE_INFO("  Unbounded array '{0}' - using bindless count: {1}", name, rbo.Count);
					}
					else
					{
						rbo.Count = 1;
					}
				}
				else
				{
					rbo.Count = static_cast<uint32_t>(arrayCount);
				}
			}

			// Add to reflection
			reflection->AddResourceBinding(rbo);

			GX_CORE_INFO("  Resource Buffer: '{0}', Set: {1}, Binding: {2}, Descriptor Type: {3}, Stage: {4}, Count: {5}",
				rbo.Name,
				rbo.Set,
				rbo.Binding,
				DescriptorTypeToString(rbo.Type),
				ShaderStageToString(rbo.Stage),
				rbo.Count);
		}
	}

	void ShaderCompiler::ExtractStructs(slang::ProgramLayout* layout, ShaderReflection* reflection)
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

	void ShaderCompiler::ProcessStructType(slang::TypeLayoutReflection* typeLayout,
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
			reflectedStruct.Size = typeLayout->getSize();

			uint32_t fieldCount = typeLayout->getFieldCount();
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

				ReflectedStructMember member;
				member.Name = fieldName;
				member.Offset = field->getOffset();
				member.Size = CalculateTypeSize(fieldType);

				reflectedStruct.Members.push_back(member);

				GX_CORE_TRACE("    Member: '{0}', Offset: {1}, Size: {2}",
					member.Name, member.Offset, member.Size);
			}

			GX_CORE_INFO("  Extracted Struct: '{0}', Size: {1}, Members: {2}",
				reflectedStruct.Name, reflectedStruct.Size, reflectedStruct.Members.size());

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

	
	uint32_t ShaderCompiler::CalculateTypeSize(slang::TypeReflection* type)
	{
		if (!type) return 0;

		auto kind = type->getKind();

		switch (kind)
		{
		case slang::TypeReflection::Kind::Scalar:
		{
			auto scalarType = type->getScalarType();
			switch (scalarType)
			{
			case slang::TypeReflection::ScalarType::Float32: return 4;
			case slang::TypeReflection::ScalarType::Float64: return 8;
			case slang::TypeReflection::ScalarType::Int32: return 4;
			case slang::TypeReflection::ScalarType::UInt32: return 4;
			case slang::TypeReflection::ScalarType::Int16: return 2;
			case slang::TypeReflection::ScalarType::UInt16: return 2;
			case slang::TypeReflection::ScalarType::Int8: return 1;
			case slang::TypeReflection::ScalarType::UInt8: return 1;
			case slang::TypeReflection::ScalarType::Bool: return 4;
			case slang::TypeReflection::ScalarType::Int64: return 8;
			case slang::TypeReflection::ScalarType::UInt64: return 8;
			default: return 0;
			}
		}

		case slang::TypeReflection::Kind::Vector:
		{
			auto elementType = type->getElementType();
			auto elementCount = type->getElementCount();
			if (elementType && elementCount > 0)
			{
				uint32_t elementSize = CalculateTypeSize(elementType);
				return elementSize * static_cast<uint32_t>(elementCount);
			}
			return 0;
		}

		case slang::TypeReflection::Kind::Matrix:
		{
			auto rowCount = type->getRowCount();
			auto colCount = type->getColumnCount();
			auto elementType = type->getElementType();

			if (elementType)
			{
				uint32_t elementSize = CalculateTypeSize(elementType);
				// Matrices are stored column-major in GLSL/Vulkan
				// Each column is aligned to 16 bytes (vec4)
				return static_cast<uint32_t>(colCount) * 16;
			}
			return static_cast<uint32_t>(rowCount * colCount) * 4; // Assume float32
		}

		case slang::TypeReflection::Kind::Array:
		{
			auto elementType = type->getElementType();
			auto elementCount = type->getElementCount();
			if (elementType && elementCount > 0)
			{
				return CalculateTypeSize(elementType) * static_cast<uint32_t>(elementCount);
			}
			return 0;
		}

		case slang::TypeReflection::Kind::Struct:
		{
			// For structs, sum up the sizes of all fields with proper alignment
			uint32_t totalSize = 0;
			uint32_t fieldCount = type->getFieldCount();
			for (uint32_t i = 0; i < fieldCount; ++i)
			{
				auto field = type->getFieldByIndex(i);
				if (field && field->getType())
				{
					totalSize += CalculateTypeSize(field->getType());
				}
			}
			return totalSize;
		}

		case slang::TypeReflection::Kind::ConstantBuffer:
		case slang::TypeReflection::Kind::ShaderStorageBuffer:
		{
			// For buffers, get the element type size
			auto elementType = type->getElementType();
			if (elementType)
			{
				return CalculateTypeSize(elementType);
			}
			return 0;
		}

		case slang::TypeReflection::Kind::ParameterBlock:
		case slang::TypeReflection::Kind::Resource:
		{
			// Parameter blocks and resources don't contribute to push constant size
			// They are descriptors, not inline data
			GX_CORE_TRACE("Skipping ParameterBlock/Resource type in size calculation");
			return 0;
		}

		case slang::TypeReflection::Kind::SamplerState:
		case slang::TypeReflection::Kind::TextureBuffer:
		{
			// Samplers and texture buffers are descriptors, not data
			return 0;
		}

		// Type kind 18 is typically a pointer type
		case static_cast<slang::TypeReflection::Kind>(18): // Pointer type
		{
			// Pointers in Vulkan are 64-bit device addresses
			GX_CORE_TRACE("Found pointer type in push constant, size: 8 bytes");
			return 8;
		}

		default:
			GX_CORE_WARN("Unknown type kind in size calculation: {0}", static_cast<int>(kind));
			return 0;
		}
	}

	ShaderStage ShaderCompiler::SlangStageToShaderStage(SlangStage stage)
	{
		switch (stage)
		{
		case SLANG_STAGE_VERTEX:
			return ShaderStage::Vertex;
		case SLANG_STAGE_FRAGMENT:
			return ShaderStage::Fragment;
		case SLANG_STAGE_COMPUTE:
			return ShaderStage::Compute;
		case SLANG_STAGE_GEOMETRY:
			return ShaderStage::Geometry;
		default:
			GX_CORE_WARN("Couldn't find the correct shader stage!");
			return ShaderStage::All;
		}
	}

	ShaderDataType ShaderCompiler::SlangTypeToShaderDataType(slang::TypeReflection* type)
	{
		if (!type) return ShaderDataType::None;

		auto kind = type->getKind();

		switch (kind)
		{
		case slang::TypeReflection::Kind::Scalar:
		{
			auto scalarType = type->getScalarType();
			switch (scalarType)
			{
			case slang::TypeReflection::ScalarType::Float32: return ShaderDataType::Float;
			case slang::TypeReflection::ScalarType::Int32: return ShaderDataType::Int;
			case slang::TypeReflection::ScalarType::Bool: return ShaderDataType::Bool;
			case slang::TypeReflection::ScalarType::UInt32: return ShaderDataType::Int;
			default: return ShaderDataType::None;
			}
		}
		case slang::TypeReflection::Kind::Vector:
		{
			auto elementType = type->getElementType();
			auto elementCount = type->getElementCount();

			if (elementType && elementType->getScalarType() == slang::TypeReflection::ScalarType::Float32)
			{
				switch (elementCount)
				{
				case 2: return ShaderDataType::Float2;
				case 3: return ShaderDataType::Float3;
				case 4: return ShaderDataType::Float4;
				default: return ShaderDataType::None;
				}
			}
			else if (elementType && elementType->getScalarType() == slang::TypeReflection::ScalarType::Int32)
			{
				switch (elementCount)
				{
				case 2: return ShaderDataType::Int2;
				case 3: return ShaderDataType::Int3;
				case 4: return ShaderDataType::Int4;
				default: return ShaderDataType::None;
				}
			}
			else if (elementType && elementType->getScalarType() == slang::TypeReflection::ScalarType::UInt32)
			{
				switch (elementCount)
				{
				case 2: return ShaderDataType::Int2;
				case 3: return ShaderDataType::Int3;
				case 4: return ShaderDataType::Int4;
				default: return ShaderDataType::None;
				}
			}
			break;
		}
		case slang::TypeReflection::Kind::Matrix:
		{
			auto rowCount = type->getRowCount();
			auto colCount = type->getColumnCount();

			if (rowCount == 3 && colCount == 3) return ShaderDataType::Mat3;
			if (rowCount == 4 && colCount == 4) return ShaderDataType::Mat4;
			break;
		}
		default:
			break;
		}

		return ShaderDataType::None;
	}

	std::string ShaderCompiler::GenerateSemanticFromName(const std::string& name)
	{
		std::string lowerName = name;
		std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);

		if (lowerName.find("pos") != std::string::npos) return "POSITION";
		if (lowerName.find("norm") != std::string::npos) return "NORMAL";
		if (lowerName.find("tex") != std::string::npos || lowerName.find("uv") != std::string::npos) return "TEXCOORD";
		if (lowerName.find("col") != std::string::npos) return "COLOR";
		if (lowerName.find("tang") != std::string::npos) return "TANGENT";
		if (lowerName.find("binorm") != std::string::npos) return "BINORMAL";

		return "TEXCOORD";
	}

	const char* ShaderCompiler::ShaderDataTypeToString(ShaderDataType type)
	{
		switch (type)
		{
		case ShaderDataType::Float: return "Float";
		case ShaderDataType::Float2: return "Float2";
		case ShaderDataType::Float3: return "Float3";
		case ShaderDataType::Float4: return "Float4";
		case ShaderDataType::Int: return "Int";
		case ShaderDataType::Int2: return "Int2";
		case ShaderDataType::Int3: return "Int3";
		case ShaderDataType::Int4: return "Int4";
		case ShaderDataType::Mat3: return "Mat3";
		case ShaderDataType::Mat4: return "Mat4";
		case ShaderDataType::Bool: return "Bool";
		default: return "Unknown";
		}
	}

	uint32_t ShaderCompiler::GetCorrectParameterSize(slang::VariableLayoutReflection* varLayout)
	{
		if (!varLayout) return 0;

		// We need both the base Type and the TypeLayout for this logic.
		slang::TypeReflection* baseType = varLayout->getType();
		slang::TypeLayoutReflection* typeLayout = varLayout->getTypeLayout();
		if (!baseType || !typeLayout) return 0;

		// Check if the reflection kind is a ConstantBuffer wrapper.
		if (typeLayout->getKind() == slang::TypeReflection::Kind::ConstantBuffer)
		{
			// If so, we need to get the type of the struct *inside* the buffer.
			slang::TypeReflection* elementType = baseType->getElementType();
			if (elementType)
			{
				// Now, call your function on the unwrapped element type.
				return CalculateTypeSize(elementType);
			}
		}

		// If it's not a wrapper, call your function on the base type directly.
		return CalculateTypeSize(baseType);
	}

}