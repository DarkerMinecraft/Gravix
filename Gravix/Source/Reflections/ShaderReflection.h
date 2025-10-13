#pragma once

#include "Renderer/Specification.h"
#include "ReflectedStruct.h"

#include <string>
#include <vector>
#include <map>
#include <ranges>

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
	};

	struct EntryPointData
	{
		std::string Name;
		ShaderStage Stage;
	};

	struct PushConstantRange
	{
		uint32_t Size = 0;
		uint32_t Offset;
	};

	class ShaderReflection
	{
	public:
		void SetShaderName(const std::string& name) { m_Name = name; }

		void AddEntryPoint(const EntryPointData& data) { m_EntryPoints.push_back(data); }
		void AddVertexAttribute(const VertexAttribute& attribute) { m_VertexAttributes.push_back(attribute); }
		void AddResourceBinding(const ShaderResourceBinding& binding) { m_ResourceBindings.push_back(binding); }
		void AddPushConstantRange(const std::string& name, const PushConstantRange& pcRange) { m_PushConstantRanges[name] = pcRange; }
		void AddReflectedStruct(const std::string& name, const ReflectedStruct& rStruct) { m_ReflectedStructs[name] = rStruct; }

		void AddDispatchGroups(const ComputeDispatchInfo& data) { m_ComputeDispatchInfo = data; }
		void SetVertexStride(uint32_t stride) { m_Stride = stride; }

		std::vector<EntryPointData>& GetEntryPoints() { return m_EntryPoints; }
		std::vector<VertexAttribute>& GetVertexAttributes() { return m_VertexAttributes; }
		std::vector<ShaderResourceBinding>& GetResourceBindings() { return m_ResourceBindings; }
		std::vector<PushConstantRange> GetPushConstantRanges() {
			return m_PushConstantRanges | std::views::values | std::ranges::to<std::vector>();
		}
		
		ReflectedStruct& GetReflectedStruct(const std::string& name) { return m_ReflectedStructs[name]; }
		bool HasReflectedStruct(const std::string& name) const { return m_ReflectedStructs.find(name) != m_ReflectedStructs.end(); }

		bool HasPushConstantRange(const std::string& name) const { return m_PushConstantRanges.find(name) != m_PushConstantRanges.end(); }

		uint32_t GetPushConstantSize() const
		{
			if (m_PushConstantRanges.empty()) return 0;

			uint32_t maxOffset = 0;
			uint32_t sizeAtMaxOffset = 0;

			for (const auto& [name, range] : m_PushConstantRanges)
			{
				if (range.Offset >= maxOffset)
				{
					maxOffset = range.Offset;
					sizeAtMaxOffset = range.Size;
				}
			}
			return maxOffset + sizeAtMaxOffset;
		}

		std::string& GetName() { return m_Name; }
		uint32_t GetVertexStride() const { return m_Stride; }
		ComputeDispatchInfo& GetComputeDispatch() { return m_ComputeDispatchInfo; }
	private:
		std::vector<EntryPointData> m_EntryPoints;
		std::vector<VertexAttribute> m_VertexAttributes;
		std::vector<ShaderResourceBinding> m_ResourceBindings;

		std::map<std::string, ReflectedStruct> m_ReflectedStructs;
		std::map<std::string, PushConstantRange> m_PushConstantRanges;

		std::string m_Name;
		uint32_t m_Stride = 0;
		ComputeDispatchInfo m_ComputeDispatchInfo;
	};

}