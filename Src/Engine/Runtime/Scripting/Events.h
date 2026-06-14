#pragma once
#include "Core/Types/Collections/Dictionary.h"
#include "Runtime/API.h"
#include "Runtime/Utilities/Variant.h"

namespace SE
{
	class ScriptingObject;
	struct ScriptingTypeHandle;

	/// <summary>
	/// The helper utility for binding and invoking scripting events (eg. used by Visual Scripting).
	/// </summary>
	class SE_API_RUNTIME ScriptingEvents
	{
	public:

		/// <summary>
		/// Global table for registered event binder methods (key is pair of type and event name, value is method that takes instance with event, object to bind and flag to bind or unbind).
		/// </summary>
		/// <remarks>
		/// Key: pair of event type, event name.
		/// Value: event binder function with parameters: event caller instance (null for static events), object to bind, true to bind/false to unbind.
		/// </remarks>
		static Dictionary<Pair<ScriptingTypeHandle, StringView>, void(*)(ScriptingObject*, void*, bool)> EventsTable;

		/// <summary>
		/// The action called when any scripting event occurs. Can be used to invoke scripting code that binded to this particular event.
		/// </summary>
		/// <remarks>
		/// Delegate parameters: event caller instance (null for static events), event invocation parameters list, event type, event name.
		/// </remarks>
		static Delegate<ScriptingObject*, Span<Variant>, ScriptingTypeHandle, StringView> Event;
	};
}
