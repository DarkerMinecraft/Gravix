#pragma once

#include <atomic>
#include <cstdint>
#include <memory>

namespace Gravix
{
	// Base class for reference counted objects
	class RefCounted
	{
	public:
		RefCounted() : m_RefCount(0) {}
		virtual ~RefCounted() = default;

		void IncRefCount() const
		{
			m_RefCount.fetch_add(1, std::memory_order_relaxed);
		}

		void DecRefCount() const
		{
			if (m_RefCount.fetch_sub(1, std::memory_order_acq_rel) == 1)
			{
				delete this;
			}
		}

		uint32_t GetRefCount() const { return m_RefCount.load(std::memory_order_relaxed); }

	private:
		mutable std::atomic<uint32_t> m_RefCount;
	};

	// Custom smart pointer for RefCounted objects only
	template<typename T>
	class RefCountedPtr
	{
	public:
		RefCountedPtr() : m_Instance(nullptr) {}

		RefCountedPtr(std::nullptr_t) : m_Instance(nullptr) {}

		RefCountedPtr(T* instance) : m_Instance(instance)
		{
			static_assert(std::is_base_of<RefCounted, T>::value, "Class is not RefCounted!");
			IncRef();
		}

		template<typename T2>
		RefCountedPtr(const RefCountedPtr<T2>& other)
		{
			m_Instance = static_cast<T*>(other.m_Instance);
			IncRef();
		}

		template<typename T2>
		RefCountedPtr(RefCountedPtr<T2>&& other)
		{
			m_Instance = static_cast<T*>(other.m_Instance);
			other.m_Instance = nullptr;
		}

		// Allow construction from std::shared_ptr for compatibility
		RefCountedPtr(const std::shared_ptr<T>& other) : m_Instance(other.get())
		{
			IncRef();
		}

		~RefCountedPtr()
		{
			DecRef();
		}

		RefCountedPtr(const RefCountedPtr<T>& other) : m_Instance(other.m_Instance)
		{
			IncRef();
		}

		RefCountedPtr& operator=(std::nullptr_t)
		{
			DecRef();
			m_Instance = nullptr;
			return *this;
		}

		RefCountedPtr& operator=(const RefCountedPtr<T>& other)
		{
			if (this == &other)
				return *this;

			DecRef();
			m_Instance = other.m_Instance;
			IncRef();
			return *this;
		}

		template<typename T2>
		RefCountedPtr& operator=(const RefCountedPtr<T2>& other)
		{
			DecRef();
			m_Instance = static_cast<T*>(other.m_Instance);
			IncRef();
			return *this;
		}

		template<typename T2>
		RefCountedPtr& operator=(RefCountedPtr<T2>&& other)
		{
			DecRef();
			m_Instance = static_cast<T*>(other.m_Instance);
			other.m_Instance = nullptr;
			return *this;
		}

		operator bool() const { return m_Instance != nullptr; }

		T* operator->() { return m_Instance; }
		const T* operator->() const { return m_Instance; }

		T& operator*() { return *m_Instance; }
		const T& operator*() const { return *m_Instance; }

		T* Raw() { return m_Instance; }
		const T* Raw() const { return m_Instance; }

		T* get() { return m_Instance; }
		const T* get() const { return m_Instance; }

		void Reset(T* instance = nullptr)
		{
			DecRef();
			m_Instance = instance;
			IncRef();
		}

		template<typename T2>
		RefCountedPtr<T2> As() const
		{
			return RefCountedPtr<T2>(*this);
		}

		template<typename... Args>
		static RefCountedPtr<T> Create(Args&&... args)
		{
			return RefCountedPtr<T>(new T(std::forward<Args>(args)...));
		}

		bool operator==(const RefCountedPtr<T>& other) const
		{
			return m_Instance == other.m_Instance;
		}

		bool operator!=(const RefCountedPtr<T>& other) const
		{
			return !(*this == other);
		}

		bool EqualsObject(const RefCountedPtr<T>& other)
		{
			if (!m_Instance || !other.m_Instance)
				return false;

			return *m_Instance == *other.m_Instance;
		}

	private:
		void IncRef() const
		{
			if (m_Instance)
				m_Instance->IncRefCount();
		}

		void DecRef() const
		{
			if (m_Instance)
				m_Instance->DecRefCount();
		}

		template<class T2>
		friend class RefCountedPtr;

		T* m_Instance;
	};

	// Ref<T> uses custom reference counting for all types
	// All types used with Ref<> should inherit from RefCounted
	template<typename T>
	using Ref = RefCountedPtr<T>;

	// Weak reference for RefCounted objects
	template<typename T>
	class WeakRef
	{
	public:
		WeakRef() = default;

		WeakRef(Ref<T> ref)
		{
			m_Instance = ref.Raw();
		}

		WeakRef(T* instance)
		{
			m_Instance = instance;
		}

		T* operator->() { return m_Instance; }
		const T* operator->() const { return m_Instance; }

		T& operator*() { return *m_Instance; }
		const T& operator*() const { return *m_Instance; }

		bool IsValid() const { return m_Instance != nullptr; }
		operator bool() const { return IsValid(); }

	private:
		T* m_Instance = nullptr;
	};

	// For non-RefCounted objects
	template<typename T>
	using Scope = std::unique_ptr<T>;

	template<typename T, typename... Args>
	constexpr Scope<T> CreateScope(Args&&... args)
	{
		return std::make_unique<T>(std::forward<Args>(args)...);
	}
}
