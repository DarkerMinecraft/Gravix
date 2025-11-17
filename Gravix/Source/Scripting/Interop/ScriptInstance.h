#pragma once

#include <string>
#include <type_traits>
#include <cstdint>

namespace Gravix
{
	class ScriptInstance
	{
	public:
		ScriptInstance() = default;

		ScriptInstance(void* handle, const std::string& typeName)
			: m_Handle(handle), m_TypeName(typeName)
		{
		}

		// Move constructor
		ScriptInstance(ScriptInstance&& other) noexcept
			: m_Handle(other.m_Handle), m_TypeName(std::move(other.m_TypeName))
		{
			other.m_Handle = nullptr;
		}

		// Move assignment
		ScriptInstance& operator=(ScriptInstance&& other) noexcept
		{
			if (this != &other)
			{
				Destroy();
				m_Handle = other.m_Handle;
				m_TypeName = std::move(other.m_TypeName);
				other.m_Handle = nullptr;
			}
			return *this;
		}

		// Delete copy constructor and copy assignment
		ScriptInstance(const ScriptInstance&) = delete;
		ScriptInstance& operator=(const ScriptInstance&) = delete;

		~ScriptInstance()
		{
			Destroy();
		}

		// Call instance method with variadic template arguments
		template<typename... Args>
		void Call(const std::string& methodName, Args... args)
		{
			if (!m_Handle)
			{
				GX_CORE_ERROR("[ScriptInstance] Cannot call method on null instance");
				return;
			}

			constexpr size_t argCount = sizeof...(Args);

			// Get the generic CallInstanceMethod function
			typedef void (*CallMethodFn)(void*, const char*, void*, int);
			static auto callMethod = (CallMethodFn)ScriptEngine::GetFunction(
				"GravixEngine.Interop.ScriptInstanceManager, GravixScripting",
				"CallInstanceMethod"
			);

			if (!callMethod)
			{
				GX_CORE_ERROR("[ScriptInstance] Failed to get CallInstanceMethod function");
				return;
			}

			// Pack arguments into an intptr_t array
			if constexpr (argCount == 0)
			{
				// No arguments
				callMethod(m_Handle, methodName.c_str(), nullptr, 0);
			}
			else
			{
				// Create array of intptr_t for arguments
				intptr_t argArray[argCount];
				PackArgs<0>(argArray, args...);

				// Call the C# method
				callMethod(m_Handle, methodName.c_str(), argArray, argCount);
			}
		}

		bool IsValid() const { return m_Handle != nullptr; }
		void* GetHandle() const { return m_Handle; }
		const std::string& GetTypeName() const { return m_TypeName; }

	private:
		// Helper to pack arguments into intptr_t array
		template<size_t Index>
		void PackArgs(intptr_t* argArray)
		{
			// Base case - no more arguments
		}

		template<size_t Index, typename T, typename... Rest>
		void PackArgs(intptr_t* argArray, T first, Rest... rest)
		{
			if constexpr (std::is_same_v<T, int>)
			{
				// Store int directly as intptr_t
				argArray[Index] = static_cast<intptr_t>(first);
			}
			else if constexpr (std::is_same_v<T, const char*>)
			{
				// Store string pointer
				argArray[Index] = reinterpret_cast<intptr_t>(first);
			}

			// Recurse for remaining arguments
			PackArgs<Index + 1>(argArray, rest...);
		}
		void Destroy()
		{
			if (m_Handle)
			{
				typedef void (*DestroyScriptFn)(void*);
				static auto destroyScript = (DestroyScriptFn)ScriptEngine::GetFunction(
					"GravixEngine.Interop.ScriptInstanceManager, GravixScripting",
					"DestroyScript"
				);
				if (destroyScript)
				{
					destroyScript(m_Handle);
				}
				m_Handle = nullptr;
			}
		}

		void* m_Handle = nullptr;
		std::string m_TypeName;
	};
}
