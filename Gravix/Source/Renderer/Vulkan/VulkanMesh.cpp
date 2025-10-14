#include "pch.h"
#include "VulkanMesh.h"

namespace Gravix
{
	VulkanMesh::VulkanMesh(Device* device, size_t vertexSize)
		: m_Device(static_cast<VulkanDevice*>(device))
		, m_VertexSize(vertexSize)
	{
		m_VertexBuffer = m_Device->CreateBuffer(
			m_VertexSize * m_VertexCapacity,
			VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
			VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
			VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
			VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			VMA_MEMORY_USAGE_GPU_ONLY
		);

		m_IndexBuffer = m_Device->CreateBuffer(
			sizeof(uint32_t) * m_IndexCapacity,
			VK_BUFFER_USAGE_INDEX_BUFFER_BIT |
			VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			VMA_MEMORY_USAGE_GPU_ONLY
		);

		UpdateVertexBufferAddress();
	}

	VulkanMesh::~VulkanMesh()
	{
		m_Device->DestroyBuffer(m_VertexBuffer);
		m_Device->DestroyBuffer(m_IndexBuffer);
	}

	void VulkanMesh::SetVertices(const std::vector<DynamicStruct>& vertices)
	{
		if (vertices.empty())
			return;

		EnsureVertexCapacity(vertices.size());

		size_t dataSize = vertices.size() * m_VertexSize;

		// Create staging buffer
		AllocatedBuffer staging = m_Device->CreateBuffer(
			dataSize,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VMA_MEMORY_USAGE_CPU_ONLY
		);

		// Copy vertex data to staging buffer
		uint8_t* stagingPtr = static_cast<uint8_t*>(staging.Info.pMappedData);
		for (size_t i = 0; i < vertices.size(); i++)
		{
			memcpy(stagingPtr + i * m_VertexSize, vertices[i].Data(), m_VertexSize);
		}

		// Transfer staging buffer to GPU buffer
		m_Device->ImmediateSubmit([&](VkCommandBuffer cmd) {
			VkBufferCopy copyRegion{};
			copyRegion.srcOffset = 0;
			copyRegion.dstOffset = 0;
			copyRegion.size = dataSize;
			vkCmdCopyBuffer(cmd, staging.Buffer, m_VertexBuffer.Buffer, 1, &copyRegion);
			});

		m_Device->DestroyBuffer(staging);
	}

	void VulkanMesh::SetIndices(std::vector<uint32_t> indices)
	{
		if (indices.empty())
			return;

		EnsureIndexCapacity(indices.size());

		size_t dataSize = indices.size() * sizeof(uint32_t);

		// Create staging buffer
		AllocatedBuffer staging = m_Device->CreateBuffer(
			dataSize,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VMA_MEMORY_USAGE_CPU_ONLY
		);

		// Copy index data to staging buffer
		memcpy(staging.Info.pMappedData, indices.data(), dataSize);

		// Transfer staging buffer to GPU buffer
		m_Device->ImmediateSubmit([&](VkCommandBuffer cmd) {
			VkBufferCopy copyRegion{};
			copyRegion.srcOffset = 0;
			copyRegion.dstOffset = 0;
			copyRegion.size = dataSize;
			vkCmdCopyBuffer(cmd, staging.Buffer, m_IndexBuffer.Buffer, 1, &copyRegion);
			});

		m_Device->DestroyBuffer(staging);

		m_IndexCount = indices.size();
	}

	void VulkanMesh::Bind(VkCommandBuffer cmdBuffer)
	{
		if (m_IndexCount > 0)
		{
			vkCmdBindIndexBuffer(cmdBuffer, m_IndexBuffer.Buffer, 0, VK_INDEX_TYPE_UINT32);
		}
	}

	void VulkanMesh::EnsureVertexCapacity(size_t requiredVertices)
	{
		if (requiredVertices <= m_VertexCapacity)
			return;

		// Calculate new capacity with 1.5x growth factor
		size_t newCapacity = requiredVertices;
		if (newCapacity < m_VertexCapacity * 3 / 2)
			newCapacity = m_VertexCapacity * 3 / 2;

		size_t newBufferSize = m_VertexSize * newCapacity;

		// Create new larger buffer
		AllocatedBuffer newBuffer = m_Device->CreateBuffer(
			newBufferSize,
			VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
			VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
			VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
			VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			VMA_MEMORY_USAGE_GPU_ONLY
		);

		// Copy existing data if any
		if (m_VertexCount > 0)
		{
			m_Device->ImmediateSubmit([&](VkCommandBuffer cmd) 
			{
				VkBufferCopy copyRegion{};
				copyRegion.srcOffset = 0;
				copyRegion.dstOffset = 0;
				copyRegion.size = m_VertexCount * m_VertexSize;
				vkCmdCopyBuffer(cmd, m_VertexBuffer.Buffer, newBuffer.Buffer, 1, &copyRegion);
			});
		}

		// Destroy old buffer and update
		m_Device->DestroyBuffer(m_VertexBuffer);

		m_VertexBuffer = newBuffer;
		m_VertexCapacity = newCapacity;

		UpdateVertexBufferAddress();
	}

	void VulkanMesh::EnsureIndexCapacity(size_t requiredIndices)
	{
		if (requiredIndices <= m_IndexCapacity)
			return;

		// Calculate new capacity with 1.5x growth factor
		size_t newCapacity = requiredIndices;
		if (newCapacity < m_IndexCapacity * 3 / 2)
			newCapacity = m_IndexCapacity * 3 / 2;

		size_t newBufferSize = sizeof(uint32_t) * newCapacity;

		// Create new larger buffer
		AllocatedBuffer newBuffer = m_Device->CreateBuffer(
			newBufferSize,
			VK_BUFFER_USAGE_INDEX_BUFFER_BIT |
			VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			VMA_MEMORY_USAGE_GPU_ONLY
		);

		// Copy existing data if any
		if (m_IndexCount > 0)
		{
			m_Device->ImmediateSubmit([&](VkCommandBuffer cmd) 
			{
				VkBufferCopy copyRegion{};
				copyRegion.srcOffset = 0;
				copyRegion.dstOffset = 0;
				copyRegion.size = m_IndexCount * sizeof(uint32_t);
				vkCmdCopyBuffer(cmd, m_IndexBuffer.Buffer, newBuffer.Buffer, 1, &copyRegion);
			});
		}

		// Destroy old buffer and update
		m_Device->DestroyBuffer(m_IndexBuffer);
		m_IndexBuffer = newBuffer;
		m_IndexCapacity = newCapacity;
	}

	void VulkanMesh::UpdateVertexBufferAddress()
	{
		VkBufferDeviceAddressInfo addressInfo{};
		addressInfo.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
		addressInfo.buffer = m_VertexBuffer.Buffer;

		m_VertexBufferAddress = vkGetBufferDeviceAddress(m_Device->GetDevice(), &addressInfo);
	}
}
