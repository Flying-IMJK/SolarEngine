#pragma once

#include "Runtime/Core/Math/Rectangle.h"
#include "Runtime/Core/Platform/CreateWindowSettings.h"
#include "Runtime/Core/Scripting/ScriptingObject.h"
#include "Runtime/Input/Enums.h"

namespace SE
{
	class Input;
	class TextureData;
	class IGuiData;

	/// <summary>
	/// Window closing reasons.
	/// </summary>
	SE_ENUM(API)
	enum class ClosingReason
	{
		/// <summary>
		/// The unknown.
		/// </summary>
		Unknown = 0,

		/// <summary>
		/// The user.
		/// </summary>
		User,

		/// <summary>
		/// The engine exit.
		/// </summary>
		EngineExit,

		/// <summary>
		/// The close event.
		/// </summary>
		CloseEvent,
	};

	/// <summary>
	/// Types of default cursors.
	/// </summary>
	enum class CursorType
	{
		/// <summary>
		/// The default.
		/// </summary>
		Default = 0,

		/// <summary>
		/// The cross.
		/// </summary>
		Cross,

		/// <summary>
		/// The hand.
		/// </summary>
		Hand,

		/// <summary>
		/// The help icon
		/// </summary>
		Help,

		/// <summary>
		/// The I beam.
		/// </summary>
		IBeam,

		/// <summary>
		/// The blocking image.
		/// </summary>
		No,

		/// <summary>
		/// The wait.
		/// </summary>
		Wait,

		/// <summary>
		/// The size all sides.
		/// </summary>
		SizeAll,

		/// <summary>
		/// The size NE-SW.
		/// </summary>
		SizeNESW,

		/// <summary>
		/// The size NS.
		/// </summary>
		SizeNS,

		/// <summary>
		/// The size NW-SE.
		/// </summary>
		SizeNWSE,

		/// <summary>
		/// The size WE.
		/// </summary>
		SizeWE,

		/// <summary>
		/// The cursor is hidden.
		/// </summary>
		Hidden,

		MAX
	};

	/// <summary>
	/// Data drag and drop effects.
	/// </summary>
	enum class DragDropEffect
	{
		/// <summary>
		/// The none.
		/// </summary>
		None = 0,

		/// <summary>
		/// The copy.
		/// </summary>
		Copy,

		/// <summary>
		/// The move.
		/// </summary>
		Move,

		/// <summary>
		/// The link.
		/// </summary>
		Link,
	};

	/// <summary>
	/// Window hit test codes. Note: they are 1:1 mapping for Win32 values.
	/// </summary>
	enum class WindowHitCodes
	{
		/// <summary>
		/// The transparent area.
		/// </summary>
		Transparent = -1,

		/// <summary>
		/// The no hit.
		/// </summary>
		NoWhere = 0,

		/// <summary>
		/// The client area.
		/// </summary>
		Client = 1,

		/// <summary>
		/// The caption area.
		/// </summary>
		Caption = 2,

		/// <summary>
		/// The system menu.
		/// </summary>
		SystemMenu = 3,

		/// <summary>
		/// The grow box
		/// </summary>
		GrowBox = 4,

		/// <summary>
		/// The menu.
		/// </summary>
		Menu = 5,

		/// <summary>
		/// The horizontal scroll.
		/// </summary>
		HScroll = 6,

		/// <summary>
		/// The vertical scroll.
		/// </summary>
		VScroll = 7,

		/// <summary>
		/// The minimize button.
		/// </summary>
		MinButton = 8,

		/// <summary>
		/// The maximize button.
		/// </summary>
		MaxButton = 9,

		/// <summary>
		/// The left side;
		/// </summary>
		Left = 10,

		/// <summary>
		/// The right side.
		/// </summary>
		Right = 11,

		/// <summary>
		/// The top side.
		/// </summary>
		Top = 12,

		/// <summary>
		/// The top left corner.
		/// </summary>
		TopLeft = 13,

		/// <summary>
		/// The top right corner.
		/// </summary>
		TopRight = 14,

		/// <summary>
		/// The bottom side.
		/// </summary>
		Bottom = 15,

		/// <summary>
		/// The bottom left corner.
		/// </summary>
		BottomLeft = 16,

		/// <summary>
		/// The bottom right corner.
		/// </summary>
		BottomRight = 17,

		/// <summary>
		/// The border.
		/// </summary>
		Border = 18,

		/// <summary>
		/// The object.
		/// </summary>
		Object = 19,

		/// <summary>
		/// The close button.
		/// </summary>
		Close = 20,

		/// <summary>
		/// The help button.
		/// </summary>
		Help = 21,
	};


	API_INJECT_CODE(cpp, "#include \"Runtime/Core/Platform/Window.h\"");

	/// <summary>
	/// Native platform window object.
	/// </summary>
	SE_CLASS(API, NoSpawn, NoConstructor, Sealed, Name="Window")
	class SE_API_RUNTIME WindowBase : public ScriptingObject
	{
		SCRIPTING_TYPE_NO_SPAWN(WindowBase);
	public:
		friend class GPUDevice;
		friend class GPUSwapChain;
	protected:
		bool m_Visible, _minimized, _maximized, m_IsClosing, m_ShowAfterFirstPaint, m_Focused, _fullscreen;
		CreateWindowSettings m_Settings;
		String m_Title;
		CursorType m_Cursor;
		Float2 m_ClientSize;
		int m_Dpi;
		float m_DpiScale;

		Float2 m_TrackingMouseOffset;
		bool m_IsUsingMouseOffset = false;
		Rectangle m_MouseOffsetScreenSize;
		bool m_IsTrackingMouse = false;
		bool m_IsHorizontalFlippingMouse = false;
		bool m_IsVerticalFlippingMouse = false;
		bool m_IsClippingCursor = false;

		explicit WindowBase(const CreateWindowSettings& settings);
		virtual ~WindowBase();

		virtual bool InitSwapChain() = 0;
		virtual void ResizeSwapChain(float width, float height) = 0;
		virtual void ReleaseSwapChain() = 0;
		virtual void FullscreenSwapChain(bool isFullscreen) = 0;

	public:
		/// <summary>
		/// Event fired when window gets shown.
		/// </summary>
		Action ShownEvent;

		/// <summary>
		/// Event fired when window gets hidden.
		/// </summary>
		Action HiddenEvent;

		/// <summary>
		/// Event fired when window gets closed.
		/// </summary>
		Action ClosedEvent;

		/// <summary>
		/// Event fired when window gets resized.
		/// </summary>
		Delegate<Float2> ResizedEvent;

		/// <summary>
		/// Event fired when window gets focused.
		/// </summary>
		Action GetFocusEvent;

		/// <summary>
		/// Event fired when window lost focus.
		/// </summary>
		Action LostFocusEvent;

		/// <summary>
		/// Event fired when window updates UI.
		/// </summary>
		Action UpdateEvent;

		/// <summary>
		/// Event fired when window draws UI.
		/// </summary>
		Action DrawEvent;

		Action Paint;

	public:
		static Window* mainWindow;

		/// <summary>
		/// Creates a graphics window that uses the managed GUI backend.
		/// </summary>
		SE_FUNCTION(API)
		static WindowBase* Create(CreateWindowSettings settings);

		/// <summary>
		/// Creates the default settings for a managed GUI window.
		/// </summary>
		SE_FUNCTION(API)
		static CreateWindowSettings CreateDefaultSettings()
		{
			return DefaultWindowSettings();
		}

		static void SetMainWindow(Window* window);
		static Window* GetMainWindow();

		// Returns true if that window is the main Engine window (works in both editor and game mode)
		bool IsMain() const;

	public:

		// Gets create window settings constant reference
		inline CreateWindowSettings& GetSettings()
		{
			return m_Settings;
		}

		/// <summary>
		/// Gets a value that indicates whether a window is in a fullscreen mode.
		/// </summary>
		bool IsFullscreen() const;

		/// <summary>
		/// Sets a value that indicates whether a window is in a fullscreen mode.
		/// </summary>
		/// <param name="isFullscreen">If set to <c>true</c> window will enter fullscreen mode, otherwise windowed mode.</param>
		virtual void SetIsFullscreen(bool isFullscreen);

		/// <summary>
		/// Sets a value that indicates whether a window is in a fullscreen mode.
		/// </summary>
		/// <param name="isFullscreen">If set to <c>true</c> window will enter fullscreen mode, otherwise windowed mode.</param>

		/// <summary>
		/// Gets a value that indicates whether a window is not in a fullscreen mode.
		/// </summary>
		inline bool IsWindowed() const
		{
			return !IsFullscreen();
		}

		/// <summary>
		/// Gets a value that indicates whether a window is visible (hidden or shown).
		/// </summary>
		SE_FUNCTION(API)
		bool IsVisible() const;

		/// <summary>
		/// Sets a value that indicates whether a window is visible (hidden or shown).
		/// </summary>
		/// <param name="isVisible">True if show window, otherwise false if hide it.</param>
		SE_FUNCTION(API)
		void SetIsVisible(bool isVisible);

		/// <summary>
		/// Gets a value that indicates whether a window is minimized.
		/// </summary>
		inline bool IsMinimized() const
		{
			return _minimized;
		}

		/// <summary>
		/// Gets a value that indicates whether a window is maximized.
		/// </summary>
		inline bool IsMaximized() const
		{
			return _maximized;
		}

		/// <summary>
		/// Gets the native window handle.
		/// </summary>
		/// <returns>The native window object handle.</returns>
		virtual void* GetNativePtr() const = 0;

	public:

		/// <summary>
		/// Performs the UI update.
		/// </summary>
		/// <param name="dt">The delta time (in seconds).</param>
		virtual void OnUpdate();

		/// <summary>
		/// Performs the window UI rendering using Render2D.
		/// </summary>
		virtual void OnDraw();

		/// <summary>
		/// Shows the window.
		/// </summary>
		SE_FUNCTION(API)
		virtual void Show();

		/// <summary>
		/// Hides the window.
		/// </summary>
		SE_FUNCTION(API)
		virtual void Hide();


		/// <summary>
		/// Minimizes the window.
		/// </summary>
		SE_FUNCTION(API)
		virtual void Minimize()
		{
		}

		/// <summary>
		/// Maximizes the window.
		/// </summary>
		SE_FUNCTION(API)
		virtual void Maximize()
		{
		}

		/// <summary>
		/// Sets the window to be borderless or not and to be fullscreen.
		/// </summary>
		/// <param name="isBorderless">Whether or not to have borders on window.</param>
		/// <param name="maximized">Whether or not to make the borderless window fullscreen (maximize to cover whole screen).</param>
		virtual void SetBorderless(bool isBorderless, bool maximized = false)
		{
		}

		/// <summary>
		/// Restores the window state before minimizing or maximizing.
		/// </summary>
		SE_FUNCTION(API)
		virtual void Restore()
		{
		}

		/// <summary>
		/// Closes the window.
		/// </summary>
		/// <param name="reason">The closing reason.</param>
		SE_FUNCTION(API)
		virtual void Close(ClosingReason reason = ClosingReason::CloseEvent);

		/// <summary>
		/// Checks if window is closed.
		/// </summary>
		virtual bool IsClosed() const
		{
			return m_IsClosing;
		}

		/// <summary>
		/// Checks if window is foreground (the window with which the user is currently working).
		/// </summary>
		virtual bool IsForegroundWindow() const;

	public:
		/// <summary>
		/// Gets the client bounds of the window (client area not including border).
		/// </summary>
		inline Rectangle GetClientBounds() const
		{
			return Rectangle(GetClientPosition(), GetClientSize());
		}

		/// <summary>
		/// Sets the client bounds of the window (client area not including border).
		/// </summary>
		/// <param name="clientArea">The client area.</param>
		virtual void SetClientBounds(const Rectangle& clientArea)
		{
		}

		/// <summary>
		/// Gets the window position (in screen coordinates).
		/// </summary>
		SE_FUNCTION(API)
		virtual Float2 GetPosition() const
		{
			return Float2::Zero;
		}

		/// <summary>
		/// Sets the window position (in screen coordinates).
		/// </summary>
		/// <param name="position">The position.</param>
		SE_FUNCTION(API)
		virtual void SetPosition(const Float2& position)
		{
		}

		/// <summary>
		/// Gets the client position of the window (client area not including border).
		/// </summary>
		inline Float2 GetClientPosition() const
		{
			return ClientToScreen(Float2::Zero);
		}

		/// <summary>
		/// Sets the client position of the window (client area not including border)
		/// </summary>
		/// <param name="position">The client area position.</param>
		virtual void SetClientPosition(const Float2& position)
		{
			SetClientBounds(Rectangle(position, GetClientSize()));
		}

		/// <summary>
		/// Gets the window size (including border).
		/// </summary>
		virtual Float2 GetSize() const
		{
			return m_ClientSize;
		}

		/// <summary>
		/// Gets the size of the client area of the window (not including border).
		/// </summary>
		SE_FUNCTION(API)
		virtual Float2 GetClientSize() const
		{
			return m_ClientSize;
		}

		/// <summary>
		/// Sets the size of the client area of the window (not including border).
		/// </summary>
		/// <param name="size">The window client area size.</param>
		SE_FUNCTION(API)
		void SetClientSize(const Float2& size)
		{
			SetClientBounds(Rectangle(GetClientPosition(), size));
		}

		/// <summary>
		/// Converts screen space location into window space coordinates.
		/// </summary>
		/// <param name="screenPos">The screen position.</param>
		/// <returns>The client space position.</returns>
		virtual Float2 ScreenToClient(const Float2& screenPos) const
		{
			return screenPos;
		}

		/// <summary>
		/// Converts window space location into screen space coordinates.
		/// </summary>
		/// <param name="clientPos">The client position.</param>
		/// <returns>The screen space position.</returns>
		virtual Float2 ClientToScreen(const Float2& clientPos) const
		{
			return clientPos;
		}

		/// <summary>
		/// Gets the window DPI setting.
		/// </summary>
		int GetDpi() const
		{
			return m_Dpi;
		}

		/// <summary>
		/// Gets the window DPI scale factor (1 is default). Includes custom DPI scale
		/// </summary>
		SE_FUNCTION(API)
		float GetDpiScale() const
		{
			return Platform::CustomDpiScale * m_DpiScale;
		}

	public:
		/// <summary>
		/// Gets the window title.
		/// </summary>
		/// <returns>The window title.</returns>
		SE_FUNCTION(API)
		virtual String GetTitle() const
		{
			return m_Title;
		}

		/// <summary>
		/// Sets the window title.
		/// </summary>
		/// <param name="title">The title.</param>
		SE_FUNCTION(API)
		virtual void SetTitle(const StringView& title)
		{
			m_Title = title;
		}

		/// <summary>
		/// Gets window opacity value (valid only for windows created with SupportsTransparency flag). Opacity values are normalized to range [0;1].
		/// </summary>
		virtual float GetOpacity() const
		{
			return 1.0f;
		}

		/// <summary>
		/// Sets window opacity value (valid only for windows created with SupportsTransparency flag). Opacity values are normalized to range [0;1].
		/// </summary>
		/// <param name="opacity">The opacity.</param>
		virtual void SetOpacity(float opacity)
		{
		}

		/// <summary>
		/// Determines whether this window is focused.
		/// </summary>
		inline bool IsFocused() const
		{
			return m_Focused;
		}

		/// <summary>
		/// Focuses this window.
		/// </summary>
		SE_FUNCTION(API)
		virtual void Focus()
		{
		}

		/// <summary>
		/// Brings window to the front of the Z order.
		/// </summary>
		/// <param name="force">True if move to the front by force, otherwise false.</param>
		virtual void BringToFront(bool force = false)
		{
		}

		/// <summary>
		/// Flashes the window to bring use attention.
		/// </summary>
		virtual void FlashWindow()
		{
		}

	public:
		/// <summary>
		/// Starts drag and drop operation
		/// </summary>
		/// <param name="data">The data.</param>
		/// <returns>The result.</returns>
		virtual DragDropEffect DoDragDrop(const StringView& data)
		{
			return DragDropEffect::None;
		}

		/// <summary>
		/// Starts the mouse tracking.
		/// </summary>
		/// <param name="useMouseScreenOffset">If set to <c>true</c> will use mouse screen offset.</param>
		virtual void StartTrackingMouse(bool useMouseScreenOffset)
		{
		}

		/// <summary>
		/// Gets the mouse tracking offset.
		/// </summary>
		Float2 GetTrackingMouseOffset() const
		{
			return m_TrackingMouseOffset;
		}

		/// <summary>
		/// Gets the value indicating whenever mouse input is tracked by this window.
		/// </summary>
		bool IsMouseTracking() const
		{
			return m_IsTrackingMouse;
		}

		/// <summary>
		/// Gets the value indicating if the mouse flipped to the other screen edge horizontally
		/// </summary>
		bool IsMouseFlippingHorizontally() const
		{
			return m_IsHorizontalFlippingMouse;
		}

		/// <summary>
		/// Gets the value indicating if the mouse flipped to the other screen edge vertically
		/// </summary>
		bool IsMouseFlippingVertically() const
		{
			return m_IsVerticalFlippingMouse;
		}

		/// <summary>
		/// Ends the mouse tracking.
		/// </summary>
		virtual void EndTrackingMouse()
		{
		}

		/// <summary>
		/// Starts the cursor clipping.
		/// </summary>
		/// <param name="bounds">The screen-space bounds that the cursor will be confined to.</param>
		virtual void StartClippingCursor(const Rectangle& bounds)
		{
		}

		/// <summary>
		/// Gets the value indicating whenever the cursor is being clipped.
		/// </summary>
		bool IsCursorClipping() const
		{
			return m_IsClippingCursor;
		}

		/// <summary>
		/// Ends the cursor clipping.
		/// </summary>
		virtual void EndClippingCursor()
		{
		}

		/// <summary>
		/// Gets the mouse cursor.
		/// </summary>
		inline CursorType GetCursor() const
		{
			return m_Cursor;
		}

		/// <summary>
		/// Sets the mouse cursor.
		/// </summary>
		/// <param name="type">The cursor type.</param>
		virtual void SetCursor(CursorType type)
		{
			m_Cursor = type;
		}

		/// <summary>
		/// Sets the window icon.
		/// </summary>
		/// <param name="icon">The icon.</param>
		virtual void SetIcon(TextureData& icon)
		{
		}

	public:
		typedef Delegate<Char> CharDelegate;
		typedef Delegate<KeyboardKeys> KeyboardDelegate;
		typedef Delegate<const Float2&, MouseButton> MouseButtonDelegate;
		typedef Delegate<const Float2&> MouseDelegate;
		typedef Delegate<const Float2&, float> MouseWheelDelegate;
		typedef Delegate<const Float2&, int32> TouchDelegate;
		typedef Delegate<IGuiData*, const Float2&, DragDropEffect&> DragDelegate;
		typedef Delegate<const Float2&, WindowHitCodes&, bool&> HitTestDelegate;
		typedef Delegate<WindowHitCodes, bool&> ButtonHitDelegate;
		typedef Delegate<ClosingReason, bool&> ClosingDelegate;

		/// <summary>
		/// Event fired on character input.
		/// </summary>
		CharDelegate CharInputEvent;
		virtual void OnCharInput(Char c);

		/// <summary>
		/// Event fired on key pressed.
		/// </summary>
		KeyboardDelegate KeyDownEvent;
		virtual void OnKeyDown(KeyboardKeys key);

		/// <summary>
		/// Event fired on key released.
		/// </summary>
		KeyboardDelegate KeyUpEvent;
		virtual void OnKeyUp(KeyboardKeys key);

		/// <summary>
		/// Event fired when mouse button goes down.
		/// </summary>
		MouseButtonDelegate MouseDownEvent;
		virtual void OnMouseDown(const Float2& mousePosition, MouseButton button);

		/// <summary>
		/// Event fired when mouse button goes up.
		/// </summary>
		MouseButtonDelegate MouseUpEvent;
		virtual void OnMouseUp(const Float2& mousePosition, MouseButton button);

		/// <summary>
		/// Event fired when mouse button double clicks.
		/// </summary>
		MouseButtonDelegate MouseDoubleClickEvent;
		virtual void OnMouseDoubleClick(const Float2& mousePosition, MouseButton button);

		/// <summary>
		/// Event fired when mouse wheel is scrolling (wheel delta is normalized).
		/// </summary>
		MouseWheelDelegate MouseWheelEvent;
		virtual void OnMouseWheel(const Float2& mousePosition, float delta);

		/// <summary>
		/// Event fired when mouse moves.
		/// </summary>
		MouseDelegate MouseMoveEvent;
		virtual void OnMouseMove(const Float2& mousePosition);

		/// <summary>
		/// Event fired when mouse leaves window.
		/// </summary>
		Action MouseLeaveEvent;
		virtual void OnMouseLeave();

		/// <summary>
		/// Event fired when touch action begins.
		/// </summary>
		TouchDelegate TouchDownEvent;
		virtual void OnTouchDown(const Float2& pointerPosition, int32 pointerIndex);

		/// <summary>
		/// Event fired when touch action moves.
		/// </summary>
		TouchDelegate TouchMoveEvent;
		virtual void OnTouchMove(const Float2& pointerPosition, int32 pointerIndex);

		/// <summary>
		/// Event fired when touch action ends.
		/// </summary>
		TouchDelegate TouchUpEvent;
		virtual void OnTouchUp(const Float2& pointerPosition, int32 pointerIndex);

		/// <summary>
		/// Event fired when drag&drop enters window.
		/// </summary>
		DragDelegate DragEnterEvent;
		virtual void OnDragEnter(IGuiData* data, const Float2& mousePosition, DragDropEffect& result);

		/// <summary>
		/// Event fired when drag&drop moves over window.
		/// </summary>
		DragDelegate DragOverEvent;
		virtual void OnDragOver(IGuiData* data, const Float2& mousePosition, DragDropEffect& result);

		/// <summary>
		/// Event fired when drag&drop ends over window with drop.
		/// </summary>
		DragDelegate DragDropEvent;
		virtual void OnDragDrop(IGuiData* data, const Float2& mousePosition, DragDropEffect& result);

		/// <summary>
		/// Event fired when drag&drop leaves window.
		/// </summary>
		Action DragLeaveEvent;
		virtual void OnDragLeave();

		/// <summary>
		/// Event fired when system tests if the specified location is part of the window.
		/// </summary>
		HitTestDelegate HitTestEvent;
		virtual void OnHitTest(const Float2& mousePosition, WindowHitCodes& result, bool& handled);

		/// <summary>
		/// Event fired when system tests if the left button hit the window for the given hit code.
		/// </summary>
		ButtonHitDelegate LeftButtonHitEvent;
		virtual void OnLeftButtonHit(WindowHitCodes hit, bool& result);

		/// <summary>
		/// Event fired when window is closing. Can be used to cancel the operation.
		/// </summary>
		ClosingDelegate ClosingEvent;
		virtual void OnClosing(ClosingReason reason, bool& cancel);

	public:
		/// <summary>
		/// Gets the text entered during the current frame (Unicode).
		/// </summary>
		/// <returns>The input text (Unicode).</returns>
		StringView GetInputText() const;

		/// <summary>
		/// Gets the key state (true if key is being pressed during this frame).
		/// </summary>
		/// <param name="key">Key ID to check</param>
		/// <returns>True while the user holds down the key identified by id</returns>
		bool GetKey(KeyboardKeys key) const;

		/// <summary>
		/// Gets the key 'down' state (true if key was pressed in this frame).
		/// </summary>
		/// <param name="key">Key ID to check</param>
		/// <returns>True during the frame the user starts pressing down the key</returns>
		bool GetKeyDown(KeyboardKeys key) const;

		/// <summary>
		/// Gets the key 'up' state (true if key was released in this frame).
		/// </summary>
		/// <param name="key">Key ID to check</param>
		/// <returns>True during the frame the user releases the key</returns>
		bool GetKeyUp(KeyboardKeys key) const;

	public:
		/// <summary>
		/// Gets the mouse position in window coordinates.
		/// </summary>
		Float2 GetMousePosition() const;

		/// <summary>
		/// Sets the mouse position in window coordinates.
		/// </summary>
		/// <param name="position">Mouse position to set on</param>
		void SetMousePosition(const Float2& position) const;

		/// <summary>
		/// Gets the mouse position change during the last frame.
		/// </summary>
		/// <returns>Mouse cursor position delta</returns>
		Float2 GetMousePositionDelta() const;

		/// <summary>
		/// Gets the mouse wheel change during the last frame.
		/// </summary>
		float GetMouseScrollDelta() const;

		/// <summary>
		/// Gets the mouse button state.
		/// </summary>
		/// <param name="button">Mouse button to check</param>
		/// <returns>True while the user holds down the button</returns>
		bool GetMouseButton(MouseButton button) const;

		/// <summary>
		/// Gets the mouse button down state.
		/// </summary>
		/// <param name="button">Mouse button to check</param>
		/// <returns>True during the frame the user starts pressing down the button</returns>
		bool GetMouseButtonDown(MouseButton button) const;

		/// <summary>
		/// Gets the mouse button up state.
		/// </summary>
		/// <param name="button">Mouse button to check</param>
		/// <returns>True during the frame the user releases the button</returns>
		bool GetMouseButtonUp(MouseButton button) const;

	public:
		void OnShow();
		virtual void OnResize(int32 width, int32 height);
		void OnClosed();
		void OnGotFocus();
		void OnLostFocus();
	};

}
