#pragma once

#include "ReflectedStruct.h"
#include <cstring>

namespace Gravix
{

	class DynamicStruct
	{
	public:
		DynamicStruct() = default;

		DynamicStruct(ReflectedStruct layout)
			: m_Layout(layout)
		{
			m_Data = m_Layout.CreateInstance();

			for (const auto& field : m_Layout.GetFields())
				m_FieldOffsets[field.Name] = field.Offset;
		}

		template<typename T>
		void Set(const std::string& field, const T& value)
		{
			auto it = m_FieldOffsets.find(field);
			if (it == m_FieldOffsets.end())
				throw std::runtime_error("Field not found: " + field);

			std::memcpy(m_Data.data() + it->second, &value, sizeof(T));
		}

		template<typename T>
		T& Get(const std::string& field)
		{
			return *reinterpret_cast<T*>(m_Data.data() + m_FieldOffsets[field]);
		}

		const void* Data() const { return m_Data.data(); }
		void* Data() { return m_Data.data(); }
		size_t Size() const { return m_Layout.GetSize(); }
	private:
		ReflectedStruct m_Layout;
		std::vector<uint8_t> m_Data;

		std::unordered_map<std::string, size_t> m_FieldOffsets;

		ReflectedStructMember GetField(const std::string& name)
		{
			for (auto f : m_Layout.GetFields())
				if (f.Name == name) return f;
			throw std::runtime_error("Field not found: " + name);
		}
	};


}
