#pragma once

#include <memory>
#include <utility>
#include <type_traits>

#ifdef ENGINE_DEBUG
#define GX_ENABLE_ASSERTS
#define GX_PROFILE
#endif

#ifdef GX_ENABLE_ASSERTS
#define GX_ASSERT(x, ...) {if(!(x)) { GX_ERROR("Assertion Failed: {0}", __VA_ARGS__); __debugbreak(); }}
#define GX_CORE_ASSERT(x, ...) {if(!(x)) { GX_CORE_ERROR("Assertion Failed: {0}", __VA_ARGS__); __debugbreak(); }}

#define GX_STATIC_ASSERT(...) { GX_ERROR("Assertion Failed: {0}", __VA_ARGS__); __debugbreak(); }
#define GX_STATIC_CORE_ASSERT(...) { GX_CORE_ERROR("Assertion Failed: {0}", __VA_ARGS__); __debugbreak(); }

#else
#define GX_ASSERT(x, ...)
#define GX_CORE_ASSERT(x, ...)

#define GX_STATIC_ASSERT(...)
#define GX_STATIC_CORE_ASSERT(...)
#endif


#define BIND_EVENT_FN(x) [this](auto&&... args) -> decltype(auto) { return this->x(std::forward<decltype(args)>(args)...); }
#define BIT(x) (1 << x)

#include "RefCounted.h"

namespace Gravix
{
	// Factory for Ref - creates custom RefCounted smart pointers
	template<typename T, typename... Args>
	inline Ref<T> CreateRef(Args&&... args)
	{
		return Ref<T>::Create(std::forward<Args>(args)...);
	}

	// Weak reference alias
	template<typename T>
	using Weak = WeakRef<T>;

	// Cast for Ref types
	template<typename T, typename U>
	inline Ref<T> Cast(const Ref<U>& other)
	{
		return other.template As<T>();
	}
}