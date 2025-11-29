#pragma once 

#include <functional>
#include <string>
#include <cstdint> 

namespace Gravix
{

	class UUID
	{
	public:
		UUID();
		UUID(uint64_t uuid);
		UUID(const UUID&) = default;

		operator uint64_t() const { return m_UUID; }
		bool operator==(const UUID& other) const { return m_UUID == other.m_UUID; }
		bool operator!=(const UUID& other) const { return !(m_UUID == other.m_UUID); }

		bool operator==(uint64_t other) const { return m_UUID == other; }
		bool operator!=(uint64_t other) const { return !(m_UUID == other); }

		bool operator==(int other) const { return m_UUID == static_cast<uint64_t>(other); }
		bool operator!=(int other) const { return !(m_UUID == static_cast<uint64_t>(other)); }

		std::string ToString() const { return std::to_string(m_UUID); }
	private:
		uint64_t m_UUID;
	};

}

namespace std
{
	template<>
	struct hash<Gravix::UUID>
	{
		std::size_t operator()(const Gravix::UUID& uuid) const
		{
			return hash<uint64_t>()((uint64_t)uuid);
		}
	};
}