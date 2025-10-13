#pragma once

#include "Renderer/Generic/MeshBuffer.h"
#include "VulkanDevice.h"

namespace Gravix 
{

	class VulkanMeshBuffer : public MeshBuffer
	{
	public:
		VulkanMeshBuffer(Device* device, ReflectedStruct vertexLayout,
			size_t maxVertices, size_t maxIndices = 0);
		~VulkanMeshBuffer() override;

		// Static mesh mode
		void SetVertices(const std::vector<DynamicStruct>& vertices) override;
		void SetIndices(std::span<uint32_t> indices) override;

		// Dynamic batch mode
		void BeginBatch() override;
		void AppendVertices(const std::vector<DynamicStruct>& vertices) override;
		void AppendIndices(std::span<uint32_t> indices) override;
		void EndBatch() override;

		// Management
		void Clear() override;

		// Query
		size_t GetVertexCount() const override { return m_VertexCount; }
		size_t GetIndexCount() const override { return m_IndexCount; }
		size_t GetVertexCapacity() const override { return m_MaxVertices; }
		size_t GetIndexCapacity() const override { return m_MaxIndices; }

		// Vertex buffer device address
		uint64_t GetVertexBufferAddress() const override { return m_VertexBufferAddress; }

		void Bind(VkCommandBuffer cmdBuffer);
	private:
		void EnsureVertexCapacity(size_t requiredVertices);
		void EnsureIndexCapacity(size_t requiredIndices);
		void UpdateVertexBufferAddress();
	private:
		VulkanDevice* m_Device;
		ReflectedStruct m_VertexLayout;
		size_t m_VertexStride;

		// Buffers
		AllocatedBuffer m_VertexBuffer;
		AllocatedBuffer m_IndexBuffer;

		// Vertex buffer device address
		uint64_t m_VertexBufferAddress = 0;

		// Mapped pointers for CPU writes
		uint8_t* m_VertexPtr = nullptr;
		uint32_t* m_IndexPtr = nullptr;

		// Current state
		size_t m_VertexCount = 0;
		size_t m_IndexCount = 0;
		size_t m_CurrentVertexOffset = 0; // In bytes
		size_t m_CurrentIndexOffset = 0;  // In bytes
		size_t m_BaseVertex = 0;

		// Capacity
		size_t m_MaxVertices;
		size_t m_MaxIndices;

		// Usage tracking
		bool m_IsStatic = false;
	};
}