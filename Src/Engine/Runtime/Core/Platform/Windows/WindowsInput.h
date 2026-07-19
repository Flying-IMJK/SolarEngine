#pragma once

#if PLATFORM_WINDOWS

#include "Runtime/API.h"
#include "Runtime/Core/Platform/Types.h"
#include "../Win32/WindowsMinimal.h"

namespace SE
{
	/// <summary>
	/// Windows platform specific implementation of the input system parts. Handles XInput devices.
	/// </summary>
	class SE_API_RUNTIME WindowsInput
	{
	public:
		static void Init();
		static void Update();
		static bool WndProc(Window* window, Windows::UINT msg, Windows::WPARAM wParam, Windows::LPARAM lParam);
	};
}

#endif
