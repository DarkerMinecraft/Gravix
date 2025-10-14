#pragma once

#include <span>
#include <vector>

#include "Reflections/DynamicStruct.h"	
#include "Reflections/ReflectedStruct.h"	

namespace Gravix 
{

	class MeshBuffer
	{
	public:
		virtual ~MeshBuffer() = default;

		// Static mesh mode - set once and keep
		virtual void SetVertices(const std::vector<DynamicStruct>& vertices) = 0;
		virtual void SetIndices(std::span<uint32_t> indices) = 0;

		// Dynamic batch mode - rebuild every frame
		virtual void AppendVertices(const std::vector<DynamicStruct>& vertices) = 0;
		virtual void AppendIndices(std::span<uint32_t> indices) = 0;

		// Management
		virtual void Clear() = 0;
		virtual void ClearVertices() = 0;

		// Query
		virtual size_t GetVertexCount() const = 0;
		virtual size_t GetIndexCount() const = 0;
		virtual size_t GetVertexCapacity() const = 0;
		virtual size_t GetIndexCapacity() const = 0;

		// Vertex buffer device address
		virtual uint64_t GetVertexBufferAddress() const = 0;

		static Ref<MeshBuffer> Create(ReflectedStruct vertexLayout, uint32_t initialVertices, uint32_t initialIndices);
	};

}