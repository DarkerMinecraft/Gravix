#pragma once

#include "Renderer/Specification.h"
#include "ReflectedStruct.h"

#include "Serialization/BinarySerializer.h"
#include "Serialization/BinaryDeserializer.h"

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

		void Serialize(BinarySerializer& serializer) 
		{
			serializer.Write(Name);
			serializer.Write(Semantic);
			serializer.Write(Location);
			serializer.Write(Offset);
			serializer.Write(static_cast<uint32_t>(Type));
			serializer.Write(Size);
			serializer.Write(Normalized);
		}

		void Deserialize(BinaryDeserializer& deserializer) 
		{
			Name = deserializer.ReadString();
			Semantic = deserializer.ReadString();
			Location = deserializer.Read<uint32_t>();
			Offset = deserializer.Read<uint32_t>();
			Type = static_cast<ShaderDataType>(deserializer.Read<uint32_t>());
			Size = deserializer.Read<uint32_t>();
			Normalized = deserializer.Read<bool>();
		}
	};

	struct ComputeDispatchInfo
	{
		uint32_t LocalSizeX = 1;
		uint32_t LocalSizeY = 1;
		uint32_t LocalSizeZ = 1;

		void Serialize(BinarySerializer& serializer) 
		{
			serializer.Write(LocalSizeX);
			serializer.Write(LocalSizeY);
			serializer.Write(LocalSizeZ);
		}

		void Deserialize(BinaryDeserializer& deserializer) 
		{
			LocalSizeX = deserializer.Read<uint32_t>();
			LocalSizeY = deserializer.Read<uint32_t>();
			LocalSizeZ = deserializer.Read<uint32_t>();
		}
	};

	struct EntryPointData
	{
		std::string Name;
		ShaderStage Stage;

		void Serialize(BinarySerializer& serializer) 
		{
			serializer.Write(Name);
			serializer.Write(static_cast<uint32_t>(Stage));
		}

		void Deserialize(BinaryDeserializer& deserializer) 
		{
			Name = deserializer.ReadString();
			Stage = static_cast<ShaderStage>(deserializer.Read<uint32_t>());
		}
	};

	struct PushConstantRange
	{
		uint32_t Size = 0;
		uint32_t Offset;

		void Serialize(BinarySerializer& serializer) 
		{
			serializer.Write(Size);
			serializer.Write(Offset);
		}

		void Deserialize(BinaryDeserializer& deserializer) 
		{
			Size = deserializer.Read<uint32_t>();
			Offset = deserializer.Read<uint32_t>();
		}
	};

	class ShaderReflection
	{
	public:
		void SetShaderName(const std::string& name) { m_Name = name; }

		void AddEntryPoint(const EntryPointData& data) { m_EntryPoints.push_back(data); }
		void AddVertexAttribute(const VertexAttribute& attribute) { m_VertexAttributes.push_back(attribute); }
		void AddPushConstantRange(const std::string& name, const PushConstantRange& pcRange) { m_PushConstantRanges[name] = pcRange; }
		void AddReflectedStruct(const std::string& name, const ReflectedStruct& rStruct) { m_ReflectedStructs[name] = rStruct; }

		void AddDispatchGroups(const ComputeDispatchInfo& data) { m_ComputeDispatchInfo = data; }
		void SetVertexStride(uint32_t stride) { m_Stride = stride; }

		void SetEntryPoints(const std::vector<EntryPointData>& entryPoints) { m_EntryPoints = entryPoints; }
		void SetVertexAttributes(const std::vector<VertexAttribute>& attributes) { m_VertexAttributes = attributes; }
		void SetPushConstantRanges(const std::map<std::string, PushConstantRange>& pcRanges) { m_PushConstantRanges = pcRanges; }
		void SetReflectedStructs(const std::map<std::string, ReflectedStruct>& structs) { m_ReflectedStructs = structs; }

		std::vector<EntryPointData>& GetEntryPoints() { return m_EntryPoints; }
		std::vector<VertexAttribute>& GetVertexAttributes() { return m_VertexAttributes; }
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

		void Serialize(BinarySerializer& serializer) 
		{
			serializer.Write(m_Name);
			serializer.Write(m_EntryPoints);
			serializer.Write(m_VertexAttributes);
			serializer.Write(m_ComputeDispatchInfo);
			serializer.Write(m_ReflectedStructs);
			serializer.Write(m_PushConstantRanges);
			serializer.Write(m_Stride);
		}

		void Deserialize(BinaryDeserializer& deserializer) 
		{
			m_Name = deserializer.ReadString();
			m_EntryPoints = deserializer.ReadVector<EntryPointData>();
			m_VertexAttributes = deserializer.ReadVector<VertexAttribute>();
			m_ComputeDispatchInfo = deserializer.Read<ComputeDispatchInfo>();
			m_ReflectedStructs = deserializer.ReadMap<std::string, ReflectedStruct>();
			m_PushConstantRanges = deserializer.ReadMap<std::string, PushConstantRange>();
			m_Stride = deserializer.Read<uint32_t>();
		}
	private:
		std::vector<EntryPointData> m_EntryPoints;
		std::vector<VertexAttribute> m_VertexAttributes;

		std::map<std::string, ReflectedStruct> m_ReflectedStructs;
		std::map<std::string, PushConstantRange> m_PushConstantRanges;

		std::string m_Name;
		uint32_t m_Stride = 0;
		ComputeDispatchInfo m_ComputeDispatchInfo;
	};

}