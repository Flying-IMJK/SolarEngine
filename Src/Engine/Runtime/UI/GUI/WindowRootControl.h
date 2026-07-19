#pragma once

#include "RootControl.h"

namespace SE
{
    class GraphicWindow;

	SE_CLASS(Reflect)
	class SE_API_RUNTIME WindowRootControl : public RootControl
	{
	    SE_DEFINE_CLASS(WindowRootControl, RootControl)
	private:
        GraphicWindow* _window;
        Control* _focusedControl;
        Control* _trackingControl;

	public:

		WindowRootControl();

        /// <summary>
        /// Gets the native window object.
        /// </summary>
        GraphicWindow* Window() const { return _window; };

        /// <summary>
        /// Sets the window title.
        /// </summary>
        String GetTitle() const;
	    void SetTitle(StringView title) const;

        /// <summary>
        /// Gets a value indicating whether this window is in fullscreen mode.
        /// </summary>
        bool IsFullscreen() const;

        /// <summary>
        /// Gets a value indicating whether this window is in windowed mode.
        /// </summary>
        bool IsWindowed() const;

        /// <summary>
        /// Gets a value indicating whether this instance is visible.
        /// </summary>
        bool IsShown() const;

        /// <summary>
        /// Gets a value indicating whether this window is minimized.
        /// </summary>
        bool IsMinimized() const;

        /// <summary>
        /// Gets a value indicating whether this window is maximized.
        /// </summary>
        bool IsMaximized() const;

        explicit WindowRootControl(GraphicWindow* window);

        /// <summary>
        /// Shows the window.
        /// </summary>
        void Show() const;

        /// <summary>
        /// Hides the window.
        /// </summary>
        void Hide() const;

        /// <summary>
        /// Minimizes the window.
        /// </summary>
        void Minimize() const;

        /// <summary>
        /// Maximizes the window.
        /// </summary>
        void Maximize() const;

        /// <summary>
        /// Restores the window state before minimizing or maximizing.
        /// </summary>
        void Restore() const;

        /// <summary>
        /// Closes the window.
        /// </summary>
        /// <param name="reason">The closing reason.</param>
        void Close(ClosingReason reason = ClosingReason::CloseEvent) const;

        /// <summary>
        /// Brings window to the front of the Z order.
        /// </summary>
        /// <param name="force">True if move to the front by force, otherwise false.</param>
        void BringToFront(bool force = false) const;

        /// <summary>
        /// Flashes the window to bring use attention.
        /// </summary>
        void FlashWindow() const;

        /// <summary>
        /// Gets or sets the current focused control
        /// </summary>
        Control* GetFocusedControl() override;
	    void SetFocusedControl(Control*) override;

        /// <inheritdoc />
	    Float2 GetTrackingMouseOffset() override;

        /// <inheritdoc />
	    WindowRootControl* RootWindow() override;

        /// <inheritdoc />
	    Float2 GetMousePosition() override;
	    void SetMousePosition(Float2 value) override;

        /// <inheritdoc />
	    void StartTrackingMouse(Control* control, bool useMouseScreenOffset) override;

        /// <inheritdoc />
	    void EndTrackingMouse() override;

        /// <inheritdoc />
	    bool GetKey(KeyboardKeys key) override;

        /// <inheritdoc />
        bool GetKeyDown(KeyboardKeys key) override;

        /// <inheritdoc />
        bool GetKeyUp(KeyboardKeys key) override;

        /// <inheritdoc />
        bool GetMouseButton(MouseButton button) override;

        /// <inheritdoc />
        bool GetMouseButtonDown(MouseButton button) override;

        /// <inheritdoc />
        bool GetMouseButtonUp(MouseButton button) override;

        /// <inheritdoc />
        Float2 PointFromScreen(Float2 location) override;

        /// <inheritdoc />
        Float2 PointToScreen(Float2 location) override;

        /// <inheritdoc />
        void Focus() override;

        /// <inheritdoc />
        void DoDragDrop(DragData* data) override;

        /// <inheritdoc />
        bool OnMouseDown(Float2 location, MouseButton button) override;

        /// <inheritdoc />
        bool OnMouseUp(Float2 location, MouseButton button) override;

        /// <inheritdoc />
        bool OnMouseDoubleClick(Float2 location, MouseButton button) override;

        /// <inheritdoc />
        bool OnMouseWheel(Float2 location, float delta) override;

        /// <inheritdoc />
        void OnMouseMove(Float2 location) override;

	protected:
		/// <inheritdoc />
		bool Focus(Control* c) override;

		/// <inheritdoc />
		CursorType __GetCursor() override;
		void __SetCursor(CursorType value) override;
	};
} // SE

