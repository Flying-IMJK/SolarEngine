#pragma once

#if PLATFORM_WINDOWS

#include "Core/API.h"
#include "Core/Platform/Types.h"
#include "../Win32/WindowsMinimal.h"

namespace SE
{
	/// <summary>
	/// Windows platform specific implementation of the input system parts. Handles XInput devices.
	/// </summary>
	class SE_API_CORE WindowsInput
	{
	public:
		static void Init();
		static void Update();
		static bool WndProc(Window* window, Windows::UINT msg, Windows::WPARAM wParam, Windows::LPARAM lParam);
	};
}

#endif
