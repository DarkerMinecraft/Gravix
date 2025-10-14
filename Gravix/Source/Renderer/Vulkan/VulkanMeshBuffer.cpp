#include "pch.h"
#include "VulkanMeshBuffer.h"

namespace Gravix
{
	VulkanMeshBuffer::VulkanMeshBuffer(Device* device, ReflectedStruct vertexLayout,
		size_t initialVertices, size_t initialIndices)
		: m_Device(static_cast<VulkanDevice*>(device))
		, m_VertexLayout(vertexLayout)
		, m_VertexStride(vertexLayout.GetSize())
		, m_VertexCapacity(initialVertices)
		, m_IndexCapacity(initialIndices > 0 ? initialIndices : initialVertices * 2)
	{
		// Create initial vertex buffer - GPU only with transfer destination
		size_t vertexBufferSize = m_VertexStride * m_VertexCapacity;
		m_VertexBuffer = m_Device->CreateBuffer(
			vertexBufferSize,
			VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
			VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
			VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
			VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			VMA_MEMORY_USAGE_GPU_ONLY
		);

		UpdateVertexBufferAddress();

		// Create initial index buffer - GPU only with transfer destination
		size_t indexBufferSize = sizeof(uint32_t) * m_IndexCapacity;
		m_IndexBuffer = m_Device->CreateBuffer(
			indexBufferSize,
			VK_BUFFER_USAGE_INDEX_BUFFER_BIT |
			VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			VMA_MEMORY_USAGE_GPU_ONLY
		);
	}

	VulkanMeshBuffer::~VulkanMeshBuffer()
	{
		m_Device->DestroyBuffer(m_VertexBuffer);
		m_Device->DestroyBuffer(m_IndexBuffer);
	}

	void VulkanMeshBuffer::SetVertices(const std::vector<DynamicStruct>& vertices)
	{
		if (vertices.empty())
			return;

		EnsureVertexCapacity(vertices.size());

		size_t dataSize = vertices.size() * m_VertexStride;

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
			memcpy(stagingPtr + i * m_VertexStride, vertices[i].Data(), m_VertexStride);
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

		m_VertexCount = vertices.size();
		m_BaseVertex = 0;

		UpdateVertexBufferAddress();
	}

	void VulkanMeshBuffer::SetIndices(std::span<uint32_t> indices)
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
		m_BaseVertex = 0;
	}

	void VulkanMeshBuffer::ClearVertices()
	{
		m_VertexCount = 0;
		m_BaseVertex = 0;
	}

	void VulkanMeshBuffer::Clear()
	{
		m_VertexCount = 0;
		m_IndexCount = 0;
		m_BaseVertex = 0;
	}

	void VulkanMeshBuffer::AppendVertices(const std::vector<DynamicStruct>& vertices)
	{
		if (vertices.empty())
			return;

		size_t requiredVertices = m_VertexCount + vertices.size();
		EnsureVertexCapacity(requiredVertices);

		size_t dataSize = vertices.size() * m_VertexStride;
		size_t dstOffset = m_VertexCount * m_VertexStride;

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
			memcpy(stagingPtr + i * m_VertexStride, vertices[i].Data(), m_VertexStride);
		}

		// Transfer staging buffer to GPU buffer at offset
		m_Device->ImmediateSubmit([&](VkCommandBuffer cmd) {
			VkBufferCopy copyRegion{};
			copyRegion.srcOffset = 0;
			copyRegion.dstOffset = dstOffset;
			copyRegion.size = dataSize;
			vkCmdCopyBuffer(cmd, staging.Buffer, m_VertexBuffer.Buffer, 1, &copyRegion);
			});

		m_Device->DestroyBuffer(staging);

		m_VertexCount += vertices.size();
	}

	void VulkanMeshBuffer::AppendIndices(std::span<uint32_t> indices)
	{
		if (indices.empty())
			return;

		size_t requiredIndices = m_IndexCount + indices.size();
		EnsureIndexCapacity(requiredIndices);

		size_t dataSize = indices.size() * sizeof(uint32_t);
		size_t dstOffset = m_IndexCount * sizeof(uint32_t);

		// Create staging buffer
		AllocatedBuffer staging = m_Device->CreateBuffer(
			dataSize,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VMA_MEMORY_USAGE_CPU_ONLY
		);

		// Copy index data with base vertex offset
		uint32_t* stagingPtr = static_cast<uint32_t*>(staging.Info.pMappedData);
		for (size_t i = 0; i < indices.size(); i++)
		{
			stagingPtr[i] = indices[i] + static_cast<uint32_t>(m_BaseVertex);
		}

		// Transfer staging buffer to GPU buffer at offset
		m_Device->ImmediateSubmit([&](VkCommandBuffer cmd) {
			VkBufferCopy copyRegion{};
			copyRegion.srcOffset = 0;
			copyRegion.dstOffset = dstOffset;
			copyRegion.size = dataSize;
			vkCmdCopyBuffer(cmd, staging.Buffer, m_IndexBuffer.Buffer, 1, &copyRegion);
			});

		m_Device->DestroyBuffer(staging);

		m_IndexCount += indices.size();
		m_BaseVertex = m_VertexCount;
	}

	void VulkanMeshBuffer::Bind(VkCommandBuffer cmdBuffer)
	{
		if (m_IndexCount > 0)
		{
			vkCmdBindIndexBuffer(cmdBuffer, m_IndexBuffer.Buffer, 0, VK_INDEX_TYPE_UINT32);
		}
	}

	void VulkanMeshBuffer::EnsureVertexCapacity(size_t requiredVertices)
	{
		if (requiredVertices <= m_VertexCapacity)
			return;

		// Calculate new capacity with 1.5x growth factor
		size_t newCapacity = requiredVertices;
		if (newCapacity < m_VertexCapacity * 3 / 2)
			newCapacity = m_VertexCapacity * 3 / 2;

		size_t newBufferSize = m_VertexStride * newCapacity;

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
			m_Device->ImmediateSubmit([&](VkCommandBuffer cmd) {
				VkBufferCopy copyRegion{};
				copyRegion.srcOffset = 0;
				copyRegion.dstOffset = 0;
				copyRegion.size = m_VertexCount * m_VertexStride;
				vkCmdCopyBuffer(cmd, m_VertexBuffer.Buffer, newBuffer.Buffer, 1, &copyRegion);
				});
		}

		// Destroy old buffer and update
		m_Device->DestroyBuffer(m_VertexBuffer);
		m_VertexBuffer = newBuffer;
		m_VertexCapacity = newCapacity;

		UpdateVertexBufferAddress();
	}

	void VulkanMeshBuffer::EnsureIndexCapacity(size_t requiredIndices)
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
			m_Device->ImmediateSubmit([&](VkCommandBuffer cmd) {
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

	void VulkanMeshBuffer::UpdateVertexBufferAddress()
	{
		VkBufferDeviceAddressInfo addressInfo{};
		addressInfo.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
		addressInfo.buffer = m_VertexBuffer.Buffer;

		m_VertexBufferAddress = vkGetBufferDeviceAddress(m_Device->GetDevice(), &addressInfo);
	}
}
