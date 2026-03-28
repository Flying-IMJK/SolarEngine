#pragma once

#include "Window.h"
#include "CriticalSection.h"

namespace SE
{
	/// <summary>
	/// Window objects manager service.
	/// </summary>
	class SE_API_CORE WindowsManager
	{
	public:

		/// <summary>
		/// The window objects collection mutex.
		/// </summary>
		static CriticalSection WindowsLocker;

		/// <summary>
		/// The window objects collection.
		/// </summary>
		static List<Window*, HeapAllocation> Windows;

	public:

		/// <summary>
		/// Gets the window by native handle.
		/// </summary>
		/// <param name="handle">The native window handle.</param>
		/// <returns>Found window or null if cannot find it.</returns>
		static Window* GetByNativePtr(void* handle);

	public:

		// Used by WindowBase
		static void Register(Window* win);
		static void Unregister(Window* win);

		static Window* GetForegroundWindow();
	};
}
