#include "pch.h"
#include "ShaderCompiler.h"

#include "Reflections/ShaderReflection.h"
#include "Renderer/Vulkan/VulkanRenderCaps.h"

#include <filesystem>

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
		sessionDesc.compilerOptionEntries = options.data();
		sessionDesc.compilerOptionEntryCount = static_cast<uint32_t>(options.size());

		Slang::ComPtr<ISession> session;
		m_GlobalSession->createSession(sessionDesc, session.writeRef());

		Slang::ComPtr<IModule> slangModule;
		{
			Slang::ComPtr<IBlob> diagnosticBlob;

			slangModule = session->loadModule(filePath.string().c_str(), diagnosticBlob.writeRef());
			if (diagnosticBlob)
			{
				std::string message = (char*)diagnosticBlob->getBufferPointer();
				GX_CORE_CRITICAL("Failed to load shader: {0}", message);
				return false;
			}
		}

		if (slangModule)
		{
			uint32_t entryPointCount = slangModule->getDefinedEntryPointCount();
			for (int i = 0; i < entryPointCount; i++)
			{
				IEntryPoint* entryPoint;
				{
					slangModule->getDefinedEntryPoint(i, &entryPoint);
					if (!entryPoint)
					{
						GX_CORE_CRITICAL("Failed to find entry point in {0}: ", filePath);
						return false;
					}
				}

				std::vector<IComponentType*> componentTypes =
				{
					slangModule,
				};
				componentTypes.push_back(entryPoint);

				Slang::ComPtr<IComponentType> composedProgram;
				{
					Slang::ComPtr<IBlob> diagnosticBlob;
					session->createCompositeComponentType(
						componentTypes.data(),
						componentTypes.size(),
						composedProgram.writeRef(),
						diagnosticBlob.writeRef());

					if (diagnosticBlob)
					{
						std::string message = (char*)diagnosticBlob->getBufferPointer();
						GX_CORE_CRITICAL("Failed to create composite component type: {0}", message);
						return false;
					}
				}

				Slang::ComPtr<slang::IComponentType> linkedProgram;
				{
					Slang::ComPtr<slang::IBlob> diagnosticsBlob;
					SlangResult result = composedProgram->link(
						linkedProgram.writeRef(),
						diagnosticsBlob.writeRef());

					if (diagnosticsBlob)
					{
						std::string message = (char*)diagnosticsBlob->getBufferPointer();
						GX_CORE_CRITICAL("Failed to link program: {0}", message);
						return false;
					}
				}
				Slang::ComPtr<slang::IBlob> spirvCode;
				{
					Slang::ComPtr<slang::IBlob> diagnosticsBlob;
					SlangResult result = linkedProgram->getEntryPointCode(
						0, // entryPointIndex
						0, // targetIndex
						spirvCode.writeRef(),
						diagnosticsBlob.writeRef());

					if (diagnosticsBlob)
					{
						std::string message = (char*)diagnosticsBlob->getBufferPointer();
						GX_CORE_CRITICAL("Failed to get entry point code: {0}", message);
						return false;
					}
				}

				std::vector<uint32_t> spirv;
				size_t codeSize = spirvCode->getBufferSize() / sizeof(uint32_t);
				spirv.resize(codeSize);
				std::memcpy(spirv.data(), spirvCode->getBufferPointer(), spirvCode->getBufferSize());

				if (spirv[0] != 0x07230203)
				{
					GX_CORE_ERROR("Invalid SPIRV magic number when adding module: {0:x}", spirv[0]);
					continue;
				}

				slang::ProgramLayout* layout = linkedProgram->getLayout();
				slang::EntryPointReflection* entryPointReflection = layout->getEntryPointByIndex(0);
				ShaderStage shaderStage = SlangStageToShaderStage(entryPointReflection->getStage());

				GX_CORE_INFO("Shader Reflection: {0}", filePath)
				GX_CORE_INFO("  Entry Point: {0}", entryPointReflection->getName())
				GX_CORE_INFO("  Stage: {0}", ShaderStageToString(shaderStage))

				spirvCodes->push_back(spirv);
				reflection->SetShaderName(filePath.stem().string());
				reflection->AddEntryPoint({ std::string(entryPointReflection->getName()), shaderStage });
				if (shaderStage != ShaderStage::Compute)
				{
					reflection->AddDispatchGroups({ 1, 1, 1, false });
				}
				else
				{
					ExtractComputeDispatchInfo(entryPointReflection, reflection);
				}

				IMetadata* metadata;
				linkedProgram->getEntryPointMetadata(
					0,
					0,
					&metadata);


				ExtractVertexAttributes(shaderStage, entryPointReflection, reflection);
				ExtractBuffers(shaderStage, layout, metadata, reflection);
			}

			return true;
		}

		return true;
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
			GX_CORE_WARN("Couldn't find the correct shader stage!")
			return ShaderStage::All;
		}
	}

	void ShaderCompiler::ExtractComputeDispatchInfo(slang::EntryPointReflection* entryPoint, ShaderReflection* reflection)
	{
		ComputeDispatchInfo dispatchInfo{};

		SlangUInt threadGroupSize[3];
		entryPoint->getComputeThreadGroupSize(3, threadGroupSize);

		dispatchInfo.LocalSizeX = static_cast<uint32_t>(threadGroupSize[0]);
		dispatchInfo.LocalSizeY = static_cast<uint32_t>(threadGroupSize[1]);
		dispatchInfo.LocalSizeZ = static_cast<uint32_t>(threadGroupSize[2]);
		dispatchInfo.IsValid = true;

		reflection->AddDispatchGroups(dispatchInfo);

		GX_CORE_INFO("  Compute Dispatch Info:");
		GX_CORE_INFO("    Local Size: ({}, {}, {})",
			dispatchInfo.LocalSizeX,
			dispatchInfo.LocalSizeY,
			dispatchInfo.LocalSizeZ);

	}

	void ShaderCompiler::ExtractBuffers(ShaderStage stage, slang::ProgramLayout* layout, IMetadata* metadata, ShaderReflection* reflection)
	{
		GX_CORE_INFO("  Resource Bindings:");

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

		for (uint32_t i = 0; i < structLayout->getFieldCount(); ++i)
		{
			auto field = structLayout->getFieldByIndex(i);
			if (!field) continue;

			// Basic lookup info
			const char* name = field->getVariable() ? field->getVariable()->getName() : nullptr;
			uint32_t binding = field->getBindingIndex();
			uint32_t set = field->getBindingSpace();

			// Offsets for diagnostic and decisions
			uint32_t space = field->getOffset(slang::ParameterCategory::SubElementRegisterSpace);
			uint32_t offset = field->getOffset(slang::ParameterCategory::DescriptorTableSlot);
			uint32_t pushConstantOffset = field->getOffset(slang::ParameterCategory::PushConstantBuffer);

			// Type information
			auto varTypeLayout = field->getTypeLayout();
			if (!varTypeLayout)
			{
				GX_CORE_WARN("    Field '{0}' has no type layout, skipping.", name ? name : "UNNAMED");
				continue;
			}
			auto type = varTypeLayout->getType();
			if (!type)
			{
				GX_CORE_WARN("    Field '{0}' has no type reflection, skipping.", name ? name : "UNNAMED");
				continue;
			}

			auto typeKind = type->getKind();
			auto resourceShape = type->getResourceShape();
			auto resourceAccess = type->getResourceAccess();

			// DEBUGGING: Log all resource details for easier diagnosis
			GX_CORE_TRACE("     Resource Analysis: '{0}'", name ? name : "UNNAMED");
			GX_CORE_TRACE("      Binding Index: {0}", binding);
			GX_CORE_TRACE("      Binding Space (Set): {0}", set);
			GX_CORE_TRACE("      Type Kind: {0}", (int)typeKind);
			GX_CORE_TRACE("      Space: {0}, Offset: {1}", space, offset);
			GX_CORE_TRACE("      Push Constant Offset: {0}", pushConstantOffset);
			GX_CORE_TRACE("      Resource Shape: {0}, Resource Access: {1}", (int)resourceShape, (int)resourceAccess);
			GX_CORE_TRACE("      Type Size: {0}", varTypeLayout->getSize());

			// ------------- PUSH CONSTANT DETECTION (DEFENSIVE) -------------
			// Only treat as push constant if:
			//  1) metadata says push-constant location is used
			//  2) pushConstantOffset is valid (not UINT32_MAX)
			//  3) type is CPU-addressable (scalar/vector/matrix/struct/constant buffer)
			//  4) resource shape is NONE (not a texture/buffer resource)
			bool isPushConstant = false;
			bool isPushUsed = false;

			metadata->isParameterLocationUsed(
				SlangParameterCategory::SLANG_PARAMETER_CATEGORY_PUSH_CONSTANT_BUFFER,
				space,
				offset,
				isPushUsed);

			bool isCpuAddressableKind =
				(typeKind == slang::TypeReflection::Kind::Scalar) ||
				(typeKind == slang::TypeReflection::Kind::Vector) ||
				(typeKind == slang::TypeReflection::Kind::Matrix) ||
				(typeKind == slang::TypeReflection::Kind::Struct) ||
				(typeKind == slang::TypeReflection::Kind::ConstantBuffer);

			bool resourceShapeIsNone = (resourceShape == SlangResourceShape::SLANG_RESOURCE_NONE);

			if (isPushUsed && pushConstantOffset != UINT32_MAX && isCpuAddressableKind && resourceShapeIsNone)
			{
				isPushConstant = true;
				GX_CORE_INFO("      Confirmed push constant: '{0}' at offset {1}", name ? name : "UNNAMED", pushConstantOffset);

				// Extract and store push constant
				PushConstant pc = ExtractPushConstant(field, stage);
				reflection->SetPushConstant(pc);
				continue; // don't treat push constants as descriptor bindings
			}

			// ------------- END PUSH CONSTANT DETECTION -------------

			// If type clearly indicates resource (texture/sampler/buffer) but metadata says descriptor slot unused, still skip
			bool isDescriptorUsed = false;
			metadata->isParameterLocationUsed(
				SlangParameterCategory::SLANG_PARAMETER_CATEGORY_DESCRIPTOR_TABLE_SLOT,
				space,
				offset,
				isDescriptorUsed);

			if (!isDescriptorUsed)
			{
				// If neither push-constant nor descriptor metadata indicates usage, skip quietly
				GX_CORE_TRACE("      Descriptor location for '{0}' not used (metadata). Skipping.", name ? name : "UNNAMED");
				continue;
			}

			// If there's a valid descriptor binding check it's not a stray/read-only zero
			if (binding == UINT32_MAX || set == UINT32_MAX)
			{
				GX_CORE_WARN("      Resource '{0}' has invalid binding ({1}) or set ({2}) - skipping.", name ? name : "UNNAMED", binding, set);
				continue;
			}

			// Prevent duplicate binding entries
			bool bindingExists = false;
			for (const auto& existingBinding : reflection->GetResourceBindings())
			{
				if (existingBinding.Binding == binding && existingBinding.Set == set)
				{
					GX_CORE_WARN("      Skipping duplicate binding: {0} at Set {1}, Binding {2}", name ? name : "UNNAMED", set, binding);
					bindingExists = true;
					break;
				}
			}
			if (bindingExists) continue;

			// Setup resource binding object
			ShaderResourceBinding rbo;
			rbo.Name = name ? name : "UNNAMED";
			rbo.Binding = binding;
			rbo.Set = set;
			rbo.Stage = stage;
			rbo.Count = 1;
			rbo.Type = DescriptorType::SampledImage; // default, may be overwritten below

			// ------------- DETERMINE DESCRIPTOR TYPE -------------
			// Try to map by type kind first (buffers / cbuffers / samplers)
			if (typeKind == slang::TypeReflection::Kind::ConstantBuffer)
			{
				rbo.Type = DescriptorType::UniformBuffer;
				GX_CORE_TRACE("      Identified as ConstantBuffer -> UniformBuffer");
			}
			else if (typeKind == slang::TypeReflection::Kind::ShaderStorageBuffer)
			{
				rbo.Type = DescriptorType::StorageBuffer;
				GX_CORE_TRACE("      Identified as ShaderStorageBuffer -> StorageBuffer");
			}
			else if (typeKind == slang::TypeReflection::Kind::SamplerState)
			{
				// Sampler states usually pair with images on descriptor sets, but treat as sampled image slot here.
				rbo.Type = DescriptorType::SampledImage;
				GX_CORE_TRACE("      Identified as SamplerState -> SampledImage");
			}
			else
			{
				// Use resource shape + access to classify textures and structured buffers
				switch (resourceShape)
				{
				case SlangResourceShape::SLANG_TEXTURE_2D:
				case SlangResourceShape::SLANG_TEXTURE_2D_ARRAY:
				case SlangResourceShape::SLANG_TEXTURE_CUBE:
				case SlangResourceShape::SLANG_TEXTURE_3D:
				case SlangResourceShape::SLANG_TEXTURE_1D:
				case SlangResourceShape::SLANG_TEXTURE_1D_ARRAY:
					if (resourceAccess == SLANG_RESOURCE_ACCESS_READ)
					{
						rbo.Type = DescriptorType::SampledImage;
						GX_CORE_TRACE("      Identified as Texture(Read) -> SampledImage");
					}
					else
					{
						rbo.Type = DescriptorType::StorageImage;
						GX_CORE_TRACE("      Identified as Texture(ReadWrite) -> StorageImage");
					}
					break;

				case SlangResourceShape::SLANG_STRUCTURED_BUFFER:
				case SlangResourceShape::SLANG_BYTE_ADDRESS_BUFFER:
					rbo.Type = DescriptorType::StorageBuffer;
					GX_CORE_TRACE("      Identified as Structured/ByteAddressBuffer -> StorageBuffer");
					break;

				case SlangResourceShape::SLANG_RESOURCE_NONE:
				default:
					// No explicit resource shape: fall back to name heuristics and type kind
					if (name && (strstr(name, "texture") || strstr(name, "Texture") || strstr(name, "sampler") || strstr(name, "Sampler") || strstr(name, "image") || strstr(name, "Image")))
					{
						rbo.Type = DescriptorType::SampledImage;
						GX_CORE_INFO("      Guessed '{0}' as SampledImage based on name pattern", name);
					}
					else
					{
						// Conservative default: UniformBuffer for struct-like CPU-addressable types, otherwise SampledImage
						if (isCpuAddressableKind)
						{
							rbo.Type = DescriptorType::UniformBuffer;
							GX_CORE_TRACE("      Defaulting to UniformBuffer for CPU-addressable kind.");
						}
						else
						{
							rbo.Type = DescriptorType::SampledImage;
							GX_CORE_WARN("      Unknown resource type for '{0}', defaulting to SampledImage (shape: {1})", name, (int)resourceShape);
						}
					}
					break;
				}
			}

			// ------------- ARRAY HANDLING -------------
			if (type->isArray())
			{
				SlangUInt arrayCount = type->getTotalArrayElementCount();
				if (arrayCount == 0)
				{
					// unsized arrays: for textures we may want bindless recommended count
					if (rbo.Type == DescriptorType::SampledImage)
					{
						rbo.Count = VulkanRenderCaps::GetRecommendedBindlessSampledImages();
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

			GX_CORE_INFO("        Added Resource Binding:");
			GX_CORE_INFO("        Name: {0}", rbo.Name);
			GX_CORE_INFO("        Type: {0}", DescriptorTypeToString(rbo.Type));
			GX_CORE_INFO("        Set: {0}, Binding: {1}", rbo.Set, rbo.Binding);
			GX_CORE_INFO("        Count: {0}", rbo.Count);
			GX_CORE_INFO("        Stage: {0}", ShaderStageToString(rbo.Stage));
		}
	}


	void ShaderCompiler::ExtractVertexAttributes(ShaderStage stage, slang::EntryPointReflection* entryPointReflection, ShaderReflection* reflection)
	{
		GX_CORE_INFO("  Vertex Attributes:");
		if (stage != ShaderStage::Vertex) return;

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

	PushConstant ShaderCompiler::ExtractPushConstant(slang::VariableLayoutReflection* field, ShaderStage stage)
	{
		PushConstant pushConstant = {};

		// Get the type layout for size information
		auto typeLayout = field->getTypeLayout();

		// Get size in bytes (default parameter category is Uniform = bytes)
		pushConstant.Size = static_cast<uint32_t>(typeLayout->getSize());

		// Get push constant specific offset
		pushConstant.Offset = static_cast<uint32_t>(
			field->getOffset(slang::ParameterCategory::PushConstantBuffer)
			);

		// Set the shader stage
		pushConstant.Stage = stage;

		GX_CORE_INFO("Extracted Push Constant - Size: {0}, Offset: {1}, Stage: {2}",
			pushConstant.Size, pushConstant.Offset, static_cast<int>(stage));

		return pushConstant;
	}

	// Helper function to convert Slang types to our ShaderDataType enum
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
			case slang::TypeReflection::ScalarType::UInt32: return ShaderDataType::Int; // Treat uint as int for simplicity
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
				// Treat uint vectors as int vectors for compatibility
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

	// Helper function to generate semantic names from attribute names
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

		// Default to TEXCOORD for unknown attributes
		return "TEXCOORD";
	}

	// Helper function to convert ShaderDataType to string for logging
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

}