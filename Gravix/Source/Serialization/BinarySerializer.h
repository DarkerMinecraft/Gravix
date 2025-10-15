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
	class BinarySerializer;

	// Type trait to detect Serialize method
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
		BinarySerializer(uint32_t version)
		{
			WriteHeader(version);
		}

		template<typename T>
		void Write(const T& obj)
		{
			if constexpr (has_serialize_v<T>)
			{
				const_cast<T&>(obj).Serialize(*this);
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

		template<typename T, size_t N>
		void Write(const std::array<T, N>& array)
		{
			for (size_t i = 0; i < N; i++)
			{
				Write(array[i]);
			}
		}

		template<typename T>
		void Write(const std::vector<T>& vec)
		{
			size_t count = vec.size();
			Write(count);
			for (const auto& obj : vec)
			{
				Write(obj);
			}
		}

		template<typename K, typename V>
		void Write(const std::map<K, V>& map)
		{
			size_t count = map.size();
			Write(count);
			for (const auto& [key, value] : map)
			{
				Write(key);
				Write(value);
			}
		}

		void WriteToFile(const std::filesystem::path& filePath)
		{
			std::ofstream file(filePath, std::ios::binary | std::ios::out);
			if (!file.is_open())
				throw std::runtime_error("Failed to open file for writing: " + filePath.string());

			file.write(reinterpret_cast<const char*>(m_Buffer.data()), m_Buffer.size());
			file.close();
		}
	private:
		void WriteHeader(uint32_t version)
		{
			const char magic[9] = "GRAVIXBN";

			m_Buffer.insert(m_Buffer.end(), magic, magic + 9);
			Write(version);
		}
	private:
		std::vector<uint8_t> m_Buffer;
	};

	// Optional: Concept alias if you prefer concept syntax elsewhere
	template<typename T>
	concept Serializable = has_serialize_v<T>;
}
