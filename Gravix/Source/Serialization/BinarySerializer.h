#pragma once

#include <vector>
#include <map>
#include <array>
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
	class BinarySerializer;

	// Trait to detect custom Serialize()
	template<typename T, typename = void>
	struct has_serialize
	{
	private:
		template<typename U>
		static auto test(int) -> decltype(std::declval<U>().Serialize(std::declval<BinarySerializer&>()), std::true_type{});

		template<typename>
		static std::false_type test(...);

	public:
		static constexpr bool value = decltype(test<T>(0))::value;
	};

	template<typename T>
	inline constexpr bool has_serialize_v = has_serialize<T>::value;

	class BinarySerializer
	{
	public:
		explicit BinarySerializer(uint32_t version)
		{
			WriteHeader(version);
		}

		template<typename T>
		void Write(const T& obj)
		{
			if constexpr (has_serialize_v<T>)
			{
				// Allow Serialize to mutate internal state if needed
				const_cast<T&>(obj).Serialize(*this);
			}
			else if constexpr (is_vector<T>::value)
			{
				WriteVector(obj);
			}
			else if constexpr (is_map<T>::value)
			{
				WriteMap(obj);
			}
			else if constexpr (std::is_trivially_copyable_v<T>)
			{
				const uint8_t* bytes = reinterpret_cast<const uint8_t*>(&obj);
				m_Buffer.insert(m_Buffer.end(), bytes, bytes + sizeof(T));
			}
			else
			{
				static_assert(sizeof(T) == 0, "Type is not serializable!");
			}
		}

		void Write(const std::string& str)
		{
			size_t len = str.size();
			Write(len);
			m_Buffer.insert(m_Buffer.end(), str.begin(), str.end());
		}

		// glm types
		void Write(const glm::vec2& vec)
		{
			Write(vec.x);
			Write(vec.y);
		}

		void Write(const glm::vec3& vec)
		{
			Write(vec.x);
			Write(vec.y);
			Write(vec.z);
		}

		void Write(const glm::vec4& vec)
		{
			Write(vec.x);
			Write(vec.y);
			Write(vec.z);
			Write(vec.w);
		}

		template<typename T, size_t N>
		void Write(const std::array<T, N>& array)
		{
			for (const auto& elem : array)
				Write(elem);
		}

		template<typename T>
		void WriteVector(const std::vector<T>& vec)
		{
			size_t count = vec.size();
			Write(count);
			for (const auto& elem : vec)
				Write(elem);
		}

		template<typename K, typename V>
		void WriteMap(const std::map<K, V>& map)
		{
			size_t count = map.size();
			Write(count);
			for (const auto& [key, value] : map)
			{
				Write(key);
				Write(value);
			}
		}

#ifdef GRAVIX_EDITOR_BUILD
		void WriteToFile(const std::filesystem::path& filePath)
		{
			std::ofstream file(filePath, std::ios::binary | std::ios::out);
			if (!file.is_open())
				throw std::runtime_error("Failed to open file for writing: " + filePath.string());

			file.write(reinterpret_cast<const char*>(m_Buffer.data()), static_cast<std::streamsize>(m_Buffer.size()));
			file.close();
		}
#endif

		// Get the raw buffer (for runtime PaK writing or custom I/O)
		const std::vector<uint8_t>& GetBuffer() const { return m_Buffer; }
		std::vector<uint8_t>& GetBuffer() { return m_Buffer; }

		void WriteBytes(const void* src, size_t numBytes)
		{
			const uint8_t* bytes = reinterpret_cast<const uint8_t*>(src);
			m_Buffer.insert(m_Buffer.end(), bytes, bytes + numBytes);
		}

	private:
		void WriteHeader(uint32_t version)
		{
			const char magic[9] = "GRAVIXBN";
			m_Buffer.insert(m_Buffer.end(), magic, magic + 8);
			Write(version);
		}

	private:
		std::vector<uint8_t> m_Buffer;

		// Helpers for container detection
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
	concept Serializable = has_serialize_v<T>;
}
