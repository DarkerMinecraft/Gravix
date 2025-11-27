#pragma once

#include "Renderer/Generic/Types/Mesh.h"
#include "Renderer/Vulkan/VulkanDevice.h"

namespace Gravix 
{

	class VulkanMesh : public Mesh
	{
	public:
		VulkanMesh(Device* device, size_t vertexSize, size_t vertexCapacity = 1024, size_t indexCapacity = 1024);
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

		size_t m_VertexCapacity;
		size_t m_IndexCapacity;

		uint32_t m_VertexCount = 0;
	};
}