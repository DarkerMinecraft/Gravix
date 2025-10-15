#pragma once

#include <span>
#include <vector>

#include "Reflections/DynamicStruct.h"	
#include "Reflections/ReflectedStruct.h"	

namespace Gravix 
{

	class Mesh
	{
	public:
		virtual ~Mesh() = default;

		virtual void SetVertices(const std::vector<DynamicStruct>& vertices) = 0;
		virtual void SetIndices(std::vector<uint32_t> indices) = 0;

		// Query
		virtual size_t GetIndexCount() const = 0;

		// Vertex buffer device address
		virtual uint64_t GetVertexBufferAddress() const = 0;

		static Ref<Mesh> Create(size_t vertexSize);
	};

}