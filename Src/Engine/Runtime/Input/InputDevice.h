
#pragma once

#include "Runtime/Core/Math/Vector2.h"
#include "Runtime/Core/Types/Collections/List.h"
#include "Enums.h"
#include "KeyboardKeys.h"
#include "Input.h"

namespace SE
{
	class InputSystem;

	/// <summary>
	/// Base class for all input device objects.
	/// </summary>
	class SE_API_RUNTIME InputDevice// : public ScriptingObject
	{
	public:
		friend class InputSystem;

	protected:
		String _name;
		InputEventQueue _queue;

		explicit InputDevice(const StringView& name)
			: _name(name)
		{
		}

	public:
		/// <summary>
		/// Gets the name.
		/// </summary>
		FORCE_INLINE const String& GetName() const
		{
			return _name;
		}

		/// <summary>
		/// Resets the input device state. Called when application looses focus.
		/// </summary>
		virtual void ResetState()
		{
			_queue.Clear();
		}

		/// <summary>
		/// Captures the input since the last call and triggers the input events.
		/// </summary>
		/// <param name="queue">The input events queue.</param>
		/// <returns>True if device has been disconnected, otherwise false.</returns>
		virtual bool Update(InputEventQueue& queue)
		{
			if (UpdateState())
				return true;
			queue.Add(_queue);
			_queue.Clear();
			return false;
		}

		/// <summary>
		/// Updates only the current state of the device.
		/// </summary>
		/// <returns>True if device has been disconnected, otherwise false.</returns>
		virtual bool UpdateState()
		{
			return false;
		}
	};
}