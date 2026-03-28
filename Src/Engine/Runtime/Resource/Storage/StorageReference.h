
#pragma once

#include "Storage.h"

namespace SE
{
	/// <summary>
	/// Storage container reference.
	/// </summary>
	struct SE_API_RUNTIME StorageReference
	{
	private:
		Storage* m_Storage;

	public:
		StorageReference() : m_Storage(nullptr)
		{
		}

		StorageReference(Storage* storage) : m_Storage(storage)
		{
			if (m_Storage)
			{
				m_Storage->AddRef();
			}
		}

		StorageReference(const StorageReference& other) : m_Storage(other.Get())
		{
			if (m_Storage)
			{
				m_Storage->AddRef();
			}
		}

		~StorageReference()
		{
			if (m_Storage)
			{
				m_Storage->RemoveRef();
			}
		}

	public:
		FORCE_INLINE Storage* Get() const
		{
			return m_Storage;
		}

	public:
		StorageReference& operator=(const StorageReference& other)
		{
			if (this != &other)
			{
				if (m_Storage)
				{
					m_Storage->RemoveRef();
				}
				m_Storage = other.m_Storage;
				if (m_Storage)
				{
					m_Storage->AddRef();
				}
			}
			return *this;
		}

		FORCE_INLINE operator bool() const
		{
			return m_Storage != nullptr;
		}

		FORCE_INLINE bool operator==(const StorageReference& other) const
		{
			return m_Storage == other.m_Storage;
		}

		FORCE_INLINE bool operator!=(const StorageReference& other) const
		{
			return m_Storage != other.m_Storage;
		}

		FORCE_INLINE Storage* operator->() const
		{
			return m_Storage;
		}
	};
}