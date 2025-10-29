#pragma once

#include <string>

namespace Gravix 
{

	class ManagedObject 
	{
		ManagedObject(const std::string& typeName);
		~ManagedObject() = default;

		ManagedObject(const ManagedObject&) = delete;
		ManagedObject& operator=(const ManagedObject&) = delete;

		void* GetHandle() const { return m_Handle; }
		bool IsValid() const { return m_Handle != nullptr; }

		template<typename TReturn, typename... TArgs>
		TReturn Invoke(const std::string& methodName, TArgs... args) 
		{

		}

		template<typename TReturn, typename... TArgs>
		TReturn Invoke(TArgs... args) 
		{
			return Invoke<TReturn, TArgs...>(m_TypeName, args...);
		}
	private:
		void* m_Handle = nullptr;
		std::string m_TypeName;
	};

}