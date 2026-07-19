#pragma once

#include "Runtime/Core/Math/Vector2.h"
#include "Runtime/Core/Types/Strings/String.h"

namespace SE
{

	/// <summary>
	/// 指定窗口的初始位置。
	/// </summary>
	SE_ENUM(API)
	enum class WindowStartPosition
	{
		/// <summary>
		/// The window is centered within the bounds of its parent window or center screen if has no parent window specified.
		/// </summary>
		CenterParent,

		/***
		 * 窗口以当前显示为中心，大小由Size 属性确定。
		 */
		CenterScreen,

		/***
		 * 窗口的位置由 Position 属性确定。
		 */
		Manual,
	};

	/// <summary>
	/// Settings for new window.
	/// </summary>
	SE_STRUCT(API)
	struct CreateWindowSettings
	{
		/// <summary>
		/// The native parent window pointer.
		/// </summary>
		Window* Parent = nullptr;

		/// <summary>
		/// The title.
		/// </summary>
		SE_PROPERTY(API)
		String Title;

		/// <summary>
		/// The custom start position.
		/// </summary>
		SE_PROPERTY(API)
		Float2 Position = Float2(100, 400);

		/// <summary>
		/// The client size.
		/// </summary>
		SE_PROPERTY(API)
		Float2 Size = Float2(1280, 768);

		/// <summary>
		/// The minimum size.
		/// </summary>
		SE_PROPERTY(API)
		Float2 MinimumSize = Float2(1, 1);

		/// <summary>
		/// The maximum size. Set to 0 to use unlimited size.
		/// </summary>
		SE_PROPERTY(API)
		Float2 MaximumSize = Float2(0, 0);

		/// <summary>
		/// The start position mode.
		/// </summary>
		SE_PROPERTY(API)
		WindowStartPosition StartPosition = WindowStartPosition::Manual;

		/// <summary>
		/// True if show window fullscreen on show.
		/// </summary>
		SE_PROPERTY(API)
		bool Fullscreen = false;

		/// <summary>
		/// Enable/disable window border.
		/// </summary>
		SE_PROPERTY(API)
		bool HasBorder = true;

		/// <summary>
		/// Enable/disable window transparency support. Required to change window opacity property.
		/// </summary>
		SE_PROPERTY(API)
		bool SupportsTransparency = false;

		/// <summary>
		/// True if show window on taskbar, otherwise it will be hidden.
		/// </summary>
		SE_PROPERTY(API)
		bool ShowInTaskbar = true;

		/// <summary>
		/// Auto activate window after show.
		/// </summary>
		SE_PROPERTY(API)
		bool ActivateWhenFirstShown = true;

		/// <summary>
		/// Allow window to capture input.
		/// </summary>
		SE_PROPERTY(API)
		bool AllowInput = true;

		/// <summary>
		/// Allow window minimize action.
		/// </summary>
		SE_PROPERTY(API)
		bool AllowMinimize = true;

		/// <summary>
		/// Allow window maximize action.
		/// </summary>
		SE_PROPERTY(API)
		bool AllowMaximize = true;

		/// <summary>
		/// 允许窗口执行拖放操作。
		/// </summary>
		SE_PROPERTY(API)
		bool AllowDragAndDrop = false;

		/// <summary>
		/// True if window topmost, otherwise false as default layout.
		/// </summary>
		SE_PROPERTY(API)
		bool IsTopmost = false;

		/// <summary>
		/// True if it's a regular window, false for tooltips, contextmenu and other utility windows.
		/// </summary>
		SE_PROPERTY(API)
		bool IsRegularWindow = true;

		/// <summary>
		/// Enable/disable window sizing frame.
		/// </summary>
		SE_PROPERTY(API)
		bool HasSizingFrame = true;

		/// <summary>
		/// Enable/disable window auto-show after the first paint.
		/// </summary>
		SE_PROPERTY(API)
		bool ShowAfterFirstPaint = true;

		/// <summary>
		/// Selects the GUI implementation for this window. C++ callers retain the native backend by default.
		/// </summary>
		enum class GuiBackend : byte
		{
			NativeLegacy,
			Managed,
		};

		GuiBackend Backend = GuiBackend::NativeLegacy;

		/// <summary>
		/// The custom data (platform dependant).
		/// </summary>
		void* Data = nullptr;
	};

	inline CreateWindowSettings DefaultWindowSettings()
	{
		CreateWindowSettings settings;
		settings.Position = Float2(100, 100);
		settings.Size = Float2(640, 480);
		settings.MinimumSize = Float2::One;
		settings.MaximumSize = Float2::Zero; // Unlimited size
		settings.StartPosition = WindowStartPosition::CenterParent;
		settings.HasBorder = true;
		settings.ShowInTaskbar = true;
		settings.ActivateWhenFirstShown = true;
		settings.AllowInput = true;
		settings.AllowMinimize = true;
		settings.AllowMaximize = true;
		settings.AllowDragAndDrop = true;
		settings.IsRegularWindow = true;
		settings.HasSizingFrame = true;
		settings.ShowAfterFirstPaint = true;
		return settings;
	}

}
