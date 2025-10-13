#include "pch.h"
#include "VulkanMeshBuffer.h"

namespace Gravix
{
	
	VulkanMeshBuffer::VulkanMeshBuffer(Device* device, ReflectedStruct vertexLayout,
		size_t maxVertices, size_t maxIndices)
		: m_Device(static_cast<VulkanDevice*>(device))
		, m_VertexLayout(vertexLayout)
		, m_VertexStride(vertexLayout.GetSize())
		, m_MaxVertices(maxVertices)
		, m_MaxIndices(maxIndices > 0 ? maxIndices : maxVertices * 2)
	{
		// Create vertex buffer with device address support
		size_t vertexBufferSize = m_VertexStride * m_MaxVertices;
		m_VertexBuffer = m_Device->CreateBuffer(
			vertexBufferSize,
			VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
			VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
			VMA_MEMORY_USAGE_CPU_TO_GPU
		);

		m_VertexPtr = static_cast<uint8_t*>(m_VertexBuffer.Info.pMappedData);
		UpdateVertexBufferAddress();

		// Create index buffer (traditional binding only)
		size_t indexBufferSize = sizeof(uint32_t) * m_MaxIndices;
		m_IndexBuffer = m_Device->CreateBuffer(
			indexBufferSize,
			VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
			VMA_MEMORY_USAGE_CPU_TO_GPU
		);

		m_IndexPtr = static_cast<uint32_t*>(m_IndexBuffer.Info.pMappedData);
	}

	VulkanMeshBuffer::~VulkanMeshBuffer()
	{
		m_Device->DestroyBuffer(m_VertexBuffer);
		m_Device->DestroyBuffer(m_IndexBuffer);
	}

	void VulkanMeshBuffer::SetVertices(const std::vector<DynamicStruct>& vertices)
	{
		EnsureVertexCapacity(vertices.size());

		// Copy all vertices
		m_CurrentVertexOffset = 0;
		for (const auto& vertex : vertices)
		{
			memcpy(m_VertexPtr + m_CurrentVertexOffset, vertex.Data(), m_VertexStride);
			m_CurrentVertexOffset += m_VertexStride;
		}

		m_VertexCount = vertices.size();
		m_IsStatic = true;
	}

	void VulkanMeshBuffer::SetIndices(std::span<uint32_t> indices)
	{
		EnsureIndexCapacity(indices.size());

		// Copy all indices
		memcpy(m_IndexPtr, indices.data(), indices.size() * sizeof(uint32_t));

		m_IndexCount = indices.size();
		m_CurrentIndexOffset = indices.size() * sizeof(uint32_t);
		m_IsStatic = true;
	}

	void VulkanMeshBuffer::BeginBatch()
	{
		m_CurrentVertexOffset = 0;
		m_CurrentIndexOffset = 0;
		m_VertexCount = 0;
		m_IndexCount = 0;
		m_BaseVertex = 0;
		m_IsStatic = false;
	}

	void VulkanMeshBuffer::AppendVertices(const std::vector<DynamicStruct>& vertices)
	{
		size_t requiredVertices = m_VertexCount + vertices.size();
		if (requiredVertices > m_MaxVertices)
		{
			throw std::runtime_error("VulkanMeshBuffer: Vertex buffer overflow. Need " +
				std::to_string(requiredVertices) + " but capacity is " +
				std::to_string(m_MaxVertices));
		}

		// Copy vertices directly into mapped buffer
		for (const auto& vertex : vertices)
		{
			memcpy(m_VertexPtr + m_CurrentVertexOffset, vertex.Data(), m_VertexStride);
			m_CurrentVertexOffset += m_VertexStride;
		}

		m_VertexCount += vertices.size();
	}

	void VulkanMeshBuffer::AppendIndices(std::span<uint32_t> indices)
	{
		size_t requiredIndices = m_IndexCount + indices.size();
		if (requiredIndices > m_MaxIndices)
		{
			throw std::runtime_error("VulkanMeshBuffer: Index buffer overflow. Need " +
				std::to_string(requiredIndices) + " but capacity is " +
				std::to_string(m_MaxIndices));
		}

		// Offset indices by current base vertex for correct batching
		for (size_t i = 0; i < indices.size(); ++i)
		{
			m_IndexPtr[m_IndexCount + i] = indices[i] + static_cast<uint32_t>(m_BaseVertex);
		}

		m_IndexCount += indices.size();
		m_CurrentIndexOffset += indices.size() * sizeof(uint32_t);

		// Update base vertex for next append
		m_BaseVertex = m_VertexCount;
	}

	void VulkanMeshBuffer::EndBatch()
	{
		// Optional: Add memory barrier or flush if using non-coherent memory
		// For VMA_MEMORY_USAGE_CPU_TO_GPU with coherent memory, no action needed
	}

	void VulkanMeshBuffer::Clear()
	{
		m_VertexCount = 0;
		m_IndexCount = 0;
		m_CurrentVertexOffset = 0;
		m_CurrentIndexOffset = 0;
		m_BaseVertex = 0;
	}

	void VulkanMeshBuffer::Bind(VkCommandBuffer cmdBuffer)
	{
		// Bind vertex buffer using device address (passed via push constants)
		// Index buffer uses traditional binding
		if (m_IndexCount > 0)
		{
			vkCmdBindIndexBuffer(cmdBuffer, m_IndexBuffer.Buffer, 0, VK_INDEX_TYPE_UINT32);
		}
	}

	void VulkanMeshBuffer::EnsureVertexCapacity(size_t requiredVertices)
	{
		if (requiredVertices <= m_MaxVertices)
			return;

		// Need to reallocate - save old data if needed
		size_t newCapacity = requiredVertices * 2; // Grow by 2x
		size_t newBufferSize = m_VertexStride * newCapacity;

		AllocatedBuffer newBuffer = m_Device->CreateBuffer(
			newBufferSize,
			VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
			VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
			VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
			VMA_MEMORY_USAGE_CPU_TO_GPU
		);

		// Copy existing data if any
		if (m_VertexCount > 0)
		{
			uint8_t* newPtr = static_cast<uint8_t*>(newBuffer.Info.pMappedData);
			memcpy(newPtr, m_VertexPtr, m_CurrentVertexOffset);
		}

		// Destroy old buffer and update
		m_Device->DestroyBuffer(m_VertexBuffer);
		m_VertexBuffer = newBuffer;
		m_VertexPtr = static_cast<uint8_t*>(m_VertexBuffer.Info.pMappedData);
		m_MaxVertices = newCapacity;

		UpdateVertexBufferAddress();
	}

	void VulkanMeshBuffer::EnsureIndexCapacity(size_t requiredIndices)
	{
		if (requiredIndices <= m_MaxIndices)
			return;

		// Need to reallocate
		size_t newCapacity = requiredIndices * 2; // Grow by 2x
		size_t newBufferSize = sizeof(uint32_t) * newCapacity;

		AllocatedBuffer newBuffer = m_Device->CreateBuffer(
			newBufferSize,
			VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
			VMA_MEMORY_USAGE_CPU_TO_GPU
		);

		// Copy existing data if any
		if (m_IndexCount > 0)
		{
			uint32_t* newPtr = static_cast<uint32_t*>(newBuffer.Info.pMappedData);
			memcpy(newPtr, m_IndexPtr, m_CurrentIndexOffset);
		}

		// Destroy old buffer and update
		m_Device->DestroyBuffer(m_IndexBuffer);
		m_IndexBuffer = newBuffer;
		m_IndexPtr = static_cast<uint32_t*>(m_IndexBuffer.Info.pMappedData);
		m_MaxIndices = newCapacity;
	}

	void VulkanMeshBuffer::UpdateVertexBufferAddress()
	{
		VkBufferDeviceAddressInfo addressInfo{};
		addressInfo.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
		addressInfo.pNext = nullptr;
		addressInfo.buffer = m_VertexBuffer.Buffer;

		m_VertexBufferAddress = vkGetBufferDeviceAddress(m_Device->GetDevice(), &addressInfo);
	}

}