#pragma once

#include "Core/Log.h"
#include "Generic/Framebuffer.h"

namespace Gravix
{

	enum class VertexFormat { Float, Float2, Float3, Float4, Int, Int2, Int3, Int4 };
	enum class Topology { TriangleList, TriangleStrip, LineList, LineStrip, PointList };
	enum class DescriptorType { UniformBuffer, StorageBuffer, SampledImage, StorageImage };
	enum class ShaderStage { Vertex, Fragment, Compute, Geometry, All };
	enum class Cull { None, Front, Back };
	enum class Fill { Solid, Wireframe };
	enum class CompareOp { Never, Less, Equal, LessOrEqual, Greater, NotEqual, GreaterOrEqual, Always };
	enum class Blending { None, Alphablend, Additive };

	enum class ShaderDataType
	{
		None = 0,
		Float, Float2, Float3, Float4,
		Int, Int2, Int3, Int4,
		Mat3, Mat4,
		Bool
	};

	static uint32_t ShaderDataTypeSize(ShaderDataType type)
	{
		switch (type)
		{
		case ShaderDataType::Float:     return 4;
		case ShaderDataType::Float2:    return 4 * 2;
		case ShaderDataType::Float3:    return 4 * 3;
		case ShaderDataType::Float4:    return 4 * 4;
		case ShaderDataType::Int:       return 4;
		case ShaderDataType::Int2:      return 4 * 2;
		case ShaderDataType::Int3:      return 4 * 3;
		case ShaderDataType::Int4:      return 4 * 4;
		case ShaderDataType::Mat3:      return 4 * 3 * 3;
		case ShaderDataType::Mat4:      return 4 * 4 * 4;
		case ShaderDataType::Bool:      return 1;
		default: return 0;
		}
	}

	/**
	 * @brief Texture filtering modes
	 */
	enum class TextureFilter
	{
		Nearest = 0,    // Pixelated/blocky filtering (good for pixel art)
		Linear = 1      // Smooth/blurred filtering
	};

	/**
	 * @brief Texture wrapping modes
	 */
	enum class TextureWrap
	{
		Repeat = 0,         // Tile the texture
		ClampToEdge = 1,    // Clamp to edge color
		ClampToBorder = 2   // Clamp to border color
	};

}