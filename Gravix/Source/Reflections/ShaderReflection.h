#pragma once

#include "Renderer/Specification.h"

#include <string>

namespace Gravix
{

	struct VertexAttribute
	{
		std::string Name;           // e.g., "position", "color", "texCoords"
		std::string Semantic;       // e.g., "POSITION", "COLOR", "TEXCOORD"
		uint32_t Location;          // Binding location in shader
		uint32_t Offset;            // Byte offset in vertex structure
		ShaderDataType Type;        // Float3, Float4, etc.
		uint32_t Size;              // Size in bytes
		bool Normalized;            // For integer types
	};

	struct ShaderResourceBinding
	{
		uint32_t Binding;
		uint32_t Set;
		uint32_t Count;
		ShaderStage Stage;
		DescriptorType Type;
		std::string Name;
	};

	struct ComputeDispatchInfo
	{
		uint32_t LocalSizeX = 1;
		uint32_t LocalSizeY = 1;
		uint32_t LocalSizeZ = 1;
		bool IsValid = false; // Set to true only for compute shaders
	};

	struct EntryPointData
	{
		std::string Name;
		ShaderStage Stage;
	};

	struct PushConstant
	{
		uint32_t Size = 0;
		uint32_t Offset;
		ShaderStage Stage;
	};

	class ShaderReflection
	{
	public:
		void SetShaderName(const std::string& name) { m_Name = name; }

		void AddEntryPoint(const EntryPointData& data) { m_EntryPoints.push_back(data); }
		void AddVertexAttribute(const VertexAttribute& attribute) { m_VertexAttributes.push_back(attribute); }
		void AddResourceBinding(const ShaderResourceBinding& binding) { m_ResourceBindings.push_back(binding); }

		void AddDispatchGroups(const ComputeDispatchInfo& data) { m_ComputeDispatchInfo = data; }
		void SetPushConstant(const PushConstant& pc) { m_PushConstant = pc; }
		void SetVertexStride(uint32_t stride) { m_Stride = stride; }

		std::vector<EntryPointData>& GetEntryPoints() { return m_EntryPoints; }
		std::vector<VertexAttribute>& GetVertexAttributes() { return m_VertexAttributes; }
		std::vector<ShaderResourceBinding>& GetResourceBindings() { return m_ResourceBindings; }

		std::string& GetName() { return m_Name; }
		uint32_t GetVertexStride() const { return m_Stride; }
		ComputeDispatchInfo& GetComputeDispatch() { return m_ComputeDispatchInfo; }
		PushConstant& GetPushConstant() { return m_PushConstant; }
	private:
		std::vector<EntryPointData> m_EntryPoints;
		std::vector<VertexAttribute> m_VertexAttributes;
		std::vector<ShaderResourceBinding> m_ResourceBindings;

		std::string m_Name;
		uint32_t m_Stride = 0;
		ComputeDispatchInfo m_ComputeDispatchInfo;
		PushConstant m_PushConstant;
	};

}