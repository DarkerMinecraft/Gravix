#include "pch.h"
#include "SlangTypeUtils.h"

#include "Core/Log.h"

#include <algorithm>

using namespace slang;
namespace Gravix
{
	namespace SlangTypeUtils
	{
		const char* ShaderStageToString(ShaderStage stage)
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

		const char* DescriptorTypeToString(DescriptorType type)
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

		const char* ShaderDataTypeToString(ShaderDataType type)
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

		ShaderStage SlangStageToShaderStage(SlangStage stage)
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

		ShaderDataType SlangTypeToShaderDataType(slang::TypeReflection* type)
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

		std::string GenerateSemanticFromName(const std::string& name)
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

		uint32_t CalculateTypeSize(slang::TypeReflection* type)
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

		uint32_t GetCorrectParameterSize(slang::VariableLayoutReflection* varLayout)
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
}
