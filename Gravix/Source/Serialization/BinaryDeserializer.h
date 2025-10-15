#pragma once

#include <vector>
#include <map>
#include <string>
#include <concepts>
#include <type_traits>
#include <cstring>
#include <iostream>
#include <fstream>

namespace Gravix
{
	// Forward declare the class
	class BinaryDeserializer;

	// Move concept after the class declaration, or use a trait
	template<typename T>
	struct has_deserialize
	{
	private:
		template<typename U>
		static auto test(int) -> decltype(std::declval<U>().Deserialize(std::declval<BinaryDeserializer&>()), std::true_type{});

		template<typename>
		static std::false_type test(...);

	public:
		static constexpr bool value = decltype(test<T>(0))::value;
	};

	template<typename T>
	inline constexpr bool has_deserialize_v = has_deserialize<T>::value;

	class BinaryDeserializer
	{
	public:
		BinaryDeserializer(const std::filesystem::path& filePath, uint32_t expectedVersion)
		{
			std::ifstream file(filePath, std::ios::binary | std::ios::ate);
			if (!file.is_open())
				throw std::runtime_error("Failed to open file for reading: " + filePath.string());

			std::streamsize size = file.tellg();
			file.seekg(0, std::ios::beg);

			m_Buffer.resize(size);
			file.read(reinterpret_cast<char*>(m_Buffer.data()), size);
			file.close();

			m_Data = m_Buffer.data();
			m_Offset = 0;

			const char expectedMagic[9] = "GRAVIXBN";
			char magic[9] = {};
			std::memcpy(magic, m_Data + m_Offset, 8);
			m_Offset += 8;
			int version = Read<uint32_t>();

			bool validMagic = std::memcmp(magic, expectedMagic, 8) == 0;
			bool validVersion = version == expectedVersion;

			GX_CORE_ASSERT(validMagic, "Wrong magic!");
			GX_CORE_ASSERT(validVersion, "Wrong version!");
		}

		template<typename T>
		T Read()
		{
			T value{};
			if constexpr (std::is_same_v<T, std::string>)
			{
				return ReadString();
			}
			else if constexpr (has_deserialize_v<T>)
			{
				value.Deserialize(*this);
			}
			else if constexpr (std::is_trivially_copyable_v<T>)
			{
				std::memcpy(&value, m_Data + m_Offset, sizeof(T));
				m_Offset += sizeof(T);
			}
			else
			{
				static_assert(sizeof(T) == 0, "Type is not deserializable!");
			}
			return value;
		}

		std::string ReadString()
		{
			size_t len = Read<size_t>();
			std::string str(reinterpret_cast<const char*>(m_Data + m_Offset), len);
			m_Offset += len;

			return str;
		}

		template<typename T>
		std::vector<T> ReadVector()
		{
			size_t count = Read<size_t>();
			std::vector<T> vec;
			vec.reserve(count);
			for (size_t i = 0; i < count; ++i)
				vec.push_back(Read<T>());

			return vec;
		}

		template<typename K, typename V>
		std::map<K, V> ReadMap()
		{
			size_t count = Read<size_t>();
			std::map<K, V> result;
			for (size_t i = 0; i < count; ++i)
			{
				K key = Read<K>();
				V value = Read<V>();
				result.emplace(std::move(key), std::move(value));
			}
			return result;
		}
	private:
		uint8_t* m_Data;
		std::vector<uint8_t> m_Buffer;
		uint32_t m_Offset = 0;
	};

	// If you prefer the concept, define it after the class
	template<typename T>
	concept Deserializable = has_deserialize_v<T>;
}
