#pragma once

#include "Core/API.h"
#include "Core/Types/Strings/String.h"
#include "Core/TypeSystem/IType.h"
#include "Core/Types/UID.h"

namespace SE
{
	/// <summary>
	/// Base for Engine objects.
	/// </summary>
	class SE_API_CORE Object : public IType
	{
		SE_CLASS(Object, IType)
	public:
			/*/// <summary>
			/// The object flags.
			/// </summary>
			ObjectFlags Flags = ObjectFlags::None;*/
	protected:
		UID m_InstanceId;

	public:
		Object();

		FORCE_INLINE const UID& GetInstanceID() const { return m_InstanceId; }

		/// <summary>
		/// Gets the string representation of this object.
		/// </summary>
		virtual String ToString() const
		{
			return String::Empty;
		};

		/// <summary>
		/// Deletes the object without queueing it to the ObjectsRemovalService.
		/// </summary>
		void DeleteObjectNow();

		/// <summary>
		/// Deletes the object (deferred).
		/// </summary>
		/// <param name="timeToLive">The time to live (in seconds). Use zero to kill it now.</param>
		/// <param name="useGameTime">True if unscaled game time for the object life timeout, otherwise false to use absolute time.</param>
		void DeleteObject(float timeToLive = 0.0f, bool useGameTime = false);

		/// <summary>
		/// Deletes the object. Called by the ObjectsRemovalService. Can be overriden to provide custom logic per object (cleanup, etc.).
		/// </summary>
		virtual void OnDeleteObject()
		{
			Delete(this);
		}
	};

	inline Object::Object()
	{
		m_InstanceId = UID::New();
	}

	template<class T>
	FORCE_INLINE void DeleteObjectSafe(T*& resource)
	{
		static_assert(TIsBaseOf<Object, T>::Value);
		if (resource)
		{
			resource->DeleteObjectNow();
			resource = nullptr;
		}
	}

}
