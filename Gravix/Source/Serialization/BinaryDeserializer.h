#pragma once

#include <vector>
#include <map>
#include <string>
#include <concepts>
#include <type_traits>
#include <cstring>
#include <iostream>

#ifdef GRAVIX_EDITOR_BUILD
#include <fstream>
#include <filesystem>
#endif

#include <glm/glm.hpp>

namespace Gravix
{
	class BinaryDeserializer;

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
#ifdef GRAVIX_EDITOR_BUILD
		// Editor: Deserialize from file path
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

			ValidateHeader(expectedVersion);
		}
#endif

		// Runtime: Deserialize from buffer (PaK data)
		BinaryDeserializer(const uint8_t* buffer, size_t size, uint32_t expectedVersion)
		{
			m_Buffer.resize(size);
			std::memcpy(m_Buffer.data(), buffer, size);
			m_Data = m_Buffer.data();
			m_Offset = 0;

			ValidateHeader(expectedVersion);
		}

		// Runtime: Deserialize from vector buffer
		BinaryDeserializer(const std::vector<uint8_t>& buffer, uint32_t expectedVersion)
		{
			m_Buffer = buffer;
			m_Data = m_Buffer.data();
			m_Offset = 0;

			ValidateHeader(expectedVersion);
		}

		template<typename T>
		T Read()
		{
			if constexpr (std::is_same_v<T, std::string>)
			{
				return ReadString();
			}
			else if constexpr (std::is_same_v<T, glm::vec2>)
			{
				return ReadVec2();
			}
			else if constexpr (std::is_same_v<T, glm::vec3>)
			{
				return ReadVec3();
			}
			else if constexpr (std::is_same_v<T, glm::vec4>)
			{
				return ReadVec4();
			}
			else if constexpr (is_vector<T>::value)
			{
				using ElemType = typename T::value_type;
				return ReadVector<ElemType>();
			}
			else if constexpr (is_map<T>::value)
			{
				using K = typename T::key_type;
				using V = typename T::mapped_type;
				return ReadMap<K, V>();
			}
			else if constexpr (has_deserialize_v<T>)
			{
				T value{};
				value.Deserialize(*this);
				return value;
			}
			else if constexpr (std::is_trivially_copyable_v<T>)
			{
				T value{};
				std::memcpy(&value, m_Data + m_Offset, sizeof(T));
				m_Offset += sizeof(T);
				return value;
			}
			else
			{
				static_assert(sizeof(T) == 0, "Type is not deserializable!");
			}
		}

		std::string ReadString()
		{
			size_t len = Read<size_t>();
			std::string str(reinterpret_cast<const char*>(m_Data + m_Offset), len);
			m_Offset += len;
			return str;
		}

		// glm types
		glm::vec2 ReadVec2()
		{
			glm::vec2 vec;
			vec.x = Read<float>();
			vec.y = Read<float>();
			return vec;
		}

		glm::vec3 ReadVec3()
		{
			glm::vec3 vec;
			vec.x = Read<float>();
			vec.y = Read<float>();
			vec.z = Read<float>();
			return vec;
		}

		glm::vec4 ReadVec4()
		{
			glm::vec4 vec;
			vec.x = Read<float>();
			vec.y = Read<float>();
			vec.z = Read<float>();
			vec.w = Read<float>();
			return vec;
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

		void ReadBytes(void* dest, size_t numBytes)
		{
			std::memcpy(dest, m_Data + m_Offset, numBytes);
			m_Offset += static_cast<uint32_t>(numBytes);
		}

	private:
		void ValidateHeader(uint32_t expectedVersion)
		{
			const char expectedMagic[9] = "GRAVIXBN";
			char magic[9] = {};
			std::memcpy(magic, m_Data + m_Offset, 8);
			m_Offset += 8;

			uint32_t version = Read<uint32_t>();

			bool validMagic = std::memcmp(magic, expectedMagic, 8) == 0;
			bool validVersion = version == expectedVersion;

			if (!validMagic) throw std::runtime_error("Invalid binary magic header!");
			if (!validVersion) throw std::runtime_error("Binary version mismatch!");
		}

		uint8_t* m_Data;
		std::vector<uint8_t> m_Buffer;
		uint32_t m_Offset = 0;

		// type traits for container detection
		template<typename T>
		struct is_vector : std::false_type {};

		template<typename T, typename A>
		struct is_vector<std::vector<T, A>> : std::true_type {};

		template<typename T>
		struct is_map : std::false_type {};

		template<typename K, typename V, typename C, typename A>
		struct is_map<std::map<K, V, C, A>> : std::true_type {};
	};

	template<typename T>
	concept Deserializable = has_deserialize_v<T>;
}
