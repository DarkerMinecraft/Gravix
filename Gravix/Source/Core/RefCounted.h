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

	// Smart pointer that automatically detects RefCounted vs non-RefCounted types
	// For RefCounted types: uses custom reference counting
	// For non-RefCounted types: uses std::shared_ptr internally
	template<typename T>
	class Ref
	{
	private:
		T* m_Ptr = nullptr;
		std::shared_ptr<T> m_SharedPtr; // Only used for non-RefCounted types

		// Tag dispatch helpers
		void IncRefImpl(T* ptr, std::true_type /* is RefCounted */)
		{
			if (ptr)
				static_cast<RefCounted*>(ptr)->IncRefCount();
		}

		void IncRefImpl(T* ptr, std::false_type /* not RefCounted */)
		{
			if (ptr)
				m_SharedPtr = std::shared_ptr<T>(ptr);
		}

		void DecRefImpl(std::true_type /* is RefCounted */)
		{
			if (m_Ptr)
				static_cast<RefCounted*>(m_Ptr)->DecRefCount();
		}

		void DecRefImpl(std::false_type /* not RefCounted */)
		{
			m_SharedPtr = nullptr;
		}

		void CopyFromImpl(const Ref& other, std::true_type /* is RefCounted */)
		{
			if (m_Ptr)
				static_cast<RefCounted*>(m_Ptr)->IncRefCount();
		}

		void CopyFromImpl(const Ref& other, std::false_type /* not RefCounted */)
		{
			m_SharedPtr = other.m_SharedPtr;
		}

		void MoveFromImpl(Ref& other, std::true_type /* is RefCounted */)
		{
			// Nothing special needed for RefCounted move
		}

		void MoveFromImpl(Ref& other, std::false_type /* not RefCounted */)
		{
			m_SharedPtr = std::move(other.m_SharedPtr);
		}

	public:
		Ref() = default;
		Ref(std::nullptr_t) : m_Ptr(nullptr) {}

		// Constructor from raw pointer
		Ref(T* ptr) : m_Ptr(ptr)
		{
			IncRefImpl(ptr, std::is_base_of<RefCounted, T>{});
		}

		// Copy constructor
		Ref(const Ref& other) : m_Ptr(other.m_Ptr)
		{
			CopyFromImpl(other, std::is_base_of<RefCounted, T>{});
		}

		// Move constructor
		Ref(Ref&& other) noexcept : m_Ptr(other.m_Ptr)
		{
			MoveFromImpl(other, std::is_base_of<RefCounted, T>{});
			other.m_Ptr = nullptr;
		}

		// Template copy constructor for derived types
		template<typename U>
		Ref(const Ref<U>& other) : m_Ptr(static_cast<T*>(other.m_Ptr))
		{
			if constexpr (std::is_base_of_v<RefCounted, T>)
			{
				if (m_Ptr)
					static_cast<RefCounted*>(m_Ptr)->IncRefCount();
			}
			else
			{
				m_SharedPtr = std::static_pointer_cast<T>(other.m_SharedPtr);
			}
		}

		// Template move constructor for derived types
		template<typename U>
		Ref(Ref<U>&& other) noexcept : m_Ptr(static_cast<T*>(other.m_Ptr))
		{
			if constexpr (std::is_base_of_v<RefCounted, T>)
			{
				// Nothing special for RefCounted move
			}
			else
			{
				m_SharedPtr = std::static_pointer_cast<T>(std::move(other.m_SharedPtr));
			}
			other.m_Ptr = nullptr;
		}

		~Ref()
		{
			DecRefImpl(std::is_base_of<RefCounted, T>{});
		}

		// Copy assignment
		Ref& operator=(const Ref& other)
		{
			if (this != &other)
			{
				DecRefImpl(std::is_base_of<RefCounted, T>{});
				m_Ptr = other.m_Ptr;
				CopyFromImpl(other, std::is_base_of<RefCounted, T>{});
			}
			return *this;
		}

		// Move assignment
		Ref& operator=(Ref&& other) noexcept
		{
			if (this != &other)
			{
				DecRefImpl(std::is_base_of<RefCounted, T>{});
				m_Ptr = other.m_Ptr;
				MoveFromImpl(other, std::is_base_of<RefCounted, T>{});
				other.m_Ptr = nullptr;
			}
			return *this;
		}

		// Nullptr assignment
		Ref& operator=(std::nullptr_t)
		{
			DecRefImpl(std::is_base_of<RefCounted, T>{});
			m_Ptr = nullptr;
			return *this;
		}

		// Template copy assignment
		template<typename U>
		Ref& operator=(const Ref<U>& other)
		{
			DecRefImpl(std::is_base_of<RefCounted, T>{});
			m_Ptr = static_cast<T*>(other.m_Ptr);
			if constexpr (std::is_base_of_v<RefCounted, T>)
			{
				if (m_Ptr)
					static_cast<RefCounted*>(m_Ptr)->IncRefCount();
			}
			else
			{
				m_SharedPtr = std::static_pointer_cast<T>(other.m_SharedPtr);
			}
			return *this;
		}

		// Template move assignment
		template<typename U>
		Ref& operator=(Ref<U>&& other) noexcept
		{
			DecRefImpl(std::is_base_of<RefCounted, T>{});
			m_Ptr = static_cast<T*>(other.m_Ptr);
			if constexpr (std::is_base_of_v<RefCounted, T>)
			{
				// Nothing special for RefCounted move
			}
			else
			{
				m_SharedPtr = std::static_pointer_cast<T>(std::move(other.m_SharedPtr));
			}
			other.m_Ptr = nullptr;
			return *this;
		}

		// Operators
		operator bool() const { return m_Ptr != nullptr; }
		T* operator->() { return m_Ptr; }
		const T* operator->() const { return m_Ptr; }
		T& operator*() { return *m_Ptr; }
		const T& operator*() const { return *m_Ptr; }

		T* Raw() { return m_Ptr; }
		const T* Raw() const { return m_Ptr; }
		T* get() { return m_Ptr; }
		const T* get() const { return m_Ptr; }

		void Reset(T* ptr = nullptr)
		{
			DecRefImpl(std::is_base_of<RefCounted, T>{});
			m_Ptr = ptr;
			IncRefImpl(ptr, std::is_base_of<RefCounted, T>{});
		}

		template<typename U>
		Ref<U> As() const
		{
			Ref<U> result;
			result.m_Ptr = static_cast<U*>(m_Ptr);
			if constexpr (std::is_base_of_v<RefCounted, U>)
			{
				if (result.m_Ptr)
					static_cast<RefCounted*>(result.m_Ptr)->IncRefCount();
			}
			else
			{
				result.m_SharedPtr = std::static_pointer_cast<U>(m_SharedPtr);
			}
			return result;
		}

		bool operator==(const Ref& other) const { return m_Ptr == other.m_Ptr; }
		bool operator!=(const Ref& other) const { return m_Ptr != other.m_Ptr; }
		bool operator==(std::nullptr_t) const { return m_Ptr == nullptr; }
		bool operator!=(std::nullptr_t) const { return m_Ptr != nullptr; }

		template<typename U>
		friend class Ref;
	};

	// Helper function to create Ref<T> for any type
	template<typename T, typename... Args>
	Ref<T> CreateRef(Args&&... args)
	{
		return Ref<T>(new T(std::forward<Args>(args)...));
	}

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
