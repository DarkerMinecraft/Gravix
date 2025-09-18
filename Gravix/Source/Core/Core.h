#pragma once

#include <memory>
#include <utility>

#ifdef ENGINE_DEBUG
#define EN_ENABLE_ASSERTS
#endif

#ifdef EN_ENABLE_ASSERTS
#define EN_ASSERT(x, ...) {if(!(x)) { EN_ERROR("Assertion Failed: {0}", __VA_ARGS__); __debugbreak(); }}
#define EN_CORE_ASSERT(x, ...) {if(!(x)) { EN_CORE_ERROR("Assertion Failed: {0}", __VA_ARGS__); __debugbreak(); }}
#else 
#define EN_ASSERT(x, ...)
#define EN_CORE_ASSERT(x, ...)
#endif


#define BIND_EVENT_FN(x) [this](auto&&... args) -> decltype(auto) { return this->x(std::forward<decltype(args)>(args)...); }
#define BIT(x) (1 << x)

// Forward declarations
template<typename T> using Ref = std::shared_ptr<T>;
template<typename T> using Scope = std::unique_ptr<T>;
template<typename T> using Weak = std::weak_ptr<T>;

// Factory for Ref (shared_ptr)
template<typename T, typename... Args>
inline Ref<T> CreateRef(Args&&... args)
{
	return std::make_shared<T>(std::forward<Args>(args)...);
}

// Factory for Scope (unique_ptr)
template<typename T, typename... Args>
inline Scope<T> CreateScope(Args&&... args)
{
	return std::make_unique<T>(std::forward<Args>(args)...);
}

template<typename T, typename... Args>
inline Weak<T> CreateWeak(Args&&... args)
{
	return Weak<T>(std::forward<Args>(args)...);
}