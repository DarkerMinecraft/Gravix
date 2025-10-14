#pragma once

#include "Renderer/Generic/Mesh.h"
#include "VulkanDevice.h"

namespace Gravix 
{

	class VulkanMesh : public Mesh
	{
	public:
		VulkanMesh(Device* device, size_t vertexSize);
		~VulkanMesh() override;

		virtual void SetVertices(const std::vector<DynamicStruct>& vertices) override;
		virtual void SetIndices(std::vector<uint32_t> indices) override;

		// Query
		size_t GetIndexCount() const override { return m_IndexCount; }

		// Vertex buffer device address
		uint64_t GetVertexBufferAddress() const override { return m_VertexBufferAddress; }

		void Bind(VkCommandBuffer cmdBuffer);
	private:
		void EnsureVertexCapacity(size_t requiredVertices);
		void EnsureIndexCapacity(size_t requiredIndices);

		void UpdateVertexBufferAddress();
	private:
		VulkanDevice* m_Device;
		// Buffers
		AllocatedBuffer m_VertexBuffer;
		AllocatedBuffer m_IndexBuffer;

		// Vertex buffer device address
		VkDeviceAddress m_VertexBufferAddress = 0;

		size_t m_VertexSize;
		uint32_t m_IndexCount; 

		size_t m_VertexCapacity = 1024 * 1024;
		size_t m_IndexCapacity = 1024 * 1024;

		uint32_t m_VertexCount = 0;
	};
}