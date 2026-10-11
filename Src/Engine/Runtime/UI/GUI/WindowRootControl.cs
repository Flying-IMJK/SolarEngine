#nullable disable annotations

namespace SE.GUI
{
    /// <summary>
    /// GUI root hosted by a window.
    /// </summary>
    public class WindowRootControl : RootControl
    {
        private SE.Window m_Window;
        private Control m_FocusedControl;
        private Control m_TrackingControl;

        /// <summary>
        /// Creates a root without a window. Window operations require a window-backed instance.
        /// </summary>
        public WindowRootControl()
        {
        }

        /// <summary>
        /// Gets the native window object. The parameterless constructor leaves it null.
        /// </summary>
        public SE.Window Window
        {
            get
            {
                return m_Window;
            }
        }

        /// <summary>
        /// Sets the window title.
        /// </summary>
        public string GetTitle()
        {
            return m_Window.Title;
        }

        /// <summary>
        /// Sets the window title.
        /// </summary>
        public void SetTitle(string title)
        {
            m_Window.Title = title;
        }

        /// <summary>
        /// Gets a value indicating whether this window is in fullscreen mode.
        /// </summary>
        public bool IsFullscreen()
        {
            return m_Window.IsFullscreen;
        }

        /// <summary>
        /// Gets a value indicating whether this window is in windowed mode.
        /// </summary>
        public bool IsWindowed()
        {
            return m_Window.IsWindowed;
        }

        /// <summary>
        /// Gets a value indicating whether this instance is visible.
        /// </summary>
        public bool IsShown()
        {
            return m_Window.IsVisible;
        }

        /// <summary>
        /// Gets a value indicating whether this window is minimized.
        /// </summary>
        public bool IsMinimized()
        {
            return m_Window.IsMinimized;
        }

        /// <summary>
        /// Gets a value indicating whether this window is maximized.
        /// </summary>
        public bool IsMaximized()
        {
            return m_Window.IsMaximized;
        }

        /// <summary>
        /// Creates a root backed by the supplied window. The caller must supply a usable window
        /// before calling window operations; as in the native constructor, this does not reject null.
        /// </summary>
        public WindowRootControl(SE.Window window)
        {
            m_Window = window;
            ClipChildren = false;

            if (Style.Current != null)
            {
                BackgroundColor = Style.Current.Background;
            }
        }

        /// <summary>
        /// Shows the window.
        /// </summary>
        public void Show()
        {
            m_Window.Show();
        }

        /// <summary>
        /// Hides the window.
        /// </summary>
        public void Hide()
        {
            m_Window.Hide();
        }

        /// <summary>
        /// Minimizes the window.
        /// </summary>
        public void Minimize()
        {
            m_Window.Minimize();
        }

        /// <summary>
        /// Maximizes the window.
        /// </summary>
        public void Maximize()
        {
            m_Window.Maximize();
        }

        /// <summary>
        /// Restores the window state before minimizing or maximizing.
        /// </summary>
        public void Restore()
        {
            m_Window.Restore();
        }

        /// <summary>
        /// Closes the window.
        /// </summary>
        /// <param name="reason">The closing reason.</param>
        public void Close(ClosingReason reason = ClosingReason.CloseEvent)
        {
            m_Window.Close(reason);
        }

        /// <summary>
        /// Brings window to the front of the Z order.
        /// </summary>
        /// <param name="force">True if move to the front by force, otherwise false.</param>
        public void BringToFront(bool force = false)
        {
            m_Window.BringToFront(force);
        }

        /// <summary>
        /// Flashes the window to bring use attention.
        /// </summary>
        public void FlashWindow()
        {
            m_Window.FlashWindow();
        }

        /// <summary>
        /// Gets or sets the current focused control
        /// </summary>
        public override Control GetFocusedControl()
        {
            return m_FocusedControl;
        }

        /// <inheritdoc />
        public override void SetFocusedControl(Control value)
        {
            // Preserve the native assertion, including its check of the previous focused control.
            SE.Debug.Assert(m_FocusedControl != null || m_FocusedControl.Root == this, "Invalid control to focus");
            Focus(value);
        }

        /// <inheritdoc />
        public override Float2 GetTrackingMouseOffset()
        {
            return m_Window.TrackingMouseOffset / m_Window.DpiScale;
        }

        /// <inheritdoc />
        public override WindowRootControl RootWindow()
        {
            return this;
        }

        /// <inheritdoc />
        public override Float2 GetMousePosition()
        {
            return m_Window.MousePosition / m_Window.DpiScale;
        }

        /// <inheritdoc />
        public override void SetMousePosition(Float2 value)
        {
            m_Window.MousePosition = value * m_Window.DpiScale;
        }

        /// <inheritdoc />
        public override void StartTrackingMouse(Control control, bool useMouseScreenOffset)
        {
            SE.Debug.Assert(control != null, "Invalid control to track");

            if (m_TrackingControl == control)
            {
                return;
            }

            if (m_TrackingControl != null)
            {
                EndTrackingMouse();
            }

            if (control.AutoFocus)
            {
                Focus(control);
            }

            m_TrackingControl = control;
            m_Window.StartTrackingMouse(useMouseScreenOffset);
        }

        /// <inheritdoc />
        public override void EndTrackingMouse()
        {
            if (m_TrackingControl != null)
            {
                Control control = m_TrackingControl;
                m_TrackingControl = null;
                control.OnEndMouseCapture();

                m_Window.EndTrackingMouse();
            }
        }

        /// <inheritdoc />
        public override bool GetKey(KeyboardKeys key)
        {
            return m_Window.GetKey(key);
        }

        /// <inheritdoc />
        public override bool GetKeyDown(KeyboardKeys key)
        {
            return m_Window.GetKeyDown(key);
        }

        /// <inheritdoc />
        public override bool GetKeyUp(KeyboardKeys key)
        {
            return m_Window.GetKeyUp(key);
        }

        /// <inheritdoc />
        public override bool GetMouseButton(MouseButton button)
        {
            return m_Window.GetMouseButton(button);
        }

        /// <inheritdoc />
        public override bool GetMouseButtonDown(MouseButton button)
        {
            return m_Window.GetMouseButtonDown(button);
        }

        /// <inheritdoc />
        public override bool GetMouseButtonUp(MouseButton button)
        {
            return m_Window.GetMouseButtonUp(button);
        }

        /// <inheritdoc />
        public override Float2 PointFromScreen(Float2 location)
        {
            return m_Window.ScreenToClient(location) / m_Window.DpiScale;
        }

        /// <inheritdoc />
        public override Float2 PointToScreen(Float2 location)
        {
            return m_Window.ClientToScreen(location * m_Window.DpiScale);
        }

        /// <inheritdoc />
        public override void Focus()
        {
            m_Window.Focus();
        }

        /// <inheritdoc />
        public override void DoDragDrop(DragData data)
        {
            if (data.DataType == DragDataType.Text)
            {
                // The native window drag operation is disabled in WindowRootControl.
            }
        }

        /// <inheritdoc />
        public override bool OnMouseDown(Float2 location, MouseButton button)
        {
            if (m_TrackingControl != null)
            {
                return m_TrackingControl.OnMouseDown(m_TrackingControl.PointFromWindow(location), button);
            }

            return base.OnMouseDown(location, button);
        }

        /// <inheritdoc />
        public override bool OnMouseUp(Float2 location, MouseButton button)
        {
            if (m_TrackingControl != null)
            {
                return m_TrackingControl.OnMouseUp(m_TrackingControl.PointFromWindow(location), button);
            }

            return base.OnMouseUp(location, button);
        }

        /// <inheritdoc />
        public override bool OnMouseDoubleClick(Float2 location, MouseButton button)
        {
            if (m_TrackingControl != null)
            {
                return m_TrackingControl.OnMouseDoubleClick(m_TrackingControl.PointFromWindow(location), button);
            }

            return base.OnMouseDoubleClick(location, button);
        }

        /// <inheritdoc />
        public override bool OnMouseWheel(Float2 location, float delta)
        {
            if (m_TrackingControl != null)
            {
                return m_TrackingControl.OnMouseWheel(m_TrackingControl.PointFromWindow(location), delta);
            }

            return base.OnMouseWheel(location, delta);
        }

        /// <inheritdoc />
        public override void OnMouseMove(Float2 location)
        {
            if (m_TrackingControl != null)
            {
                m_TrackingControl.OnMouseMove(m_TrackingControl.PointFromWindow(location));
                return;
            }

            base.OnMouseMove(location);
        }

        /// <inheritdoc />
        protected override bool Focus(Control control)
        {
            if (IsDisposing || m_FocusedControl == control)
            {
                return false;
            }

            // Change focused control
            Control previous = m_FocusedControl;
            m_FocusedControl = control;

            // Fire events
            if (previous != null)
            {
                previous.OnLostFocus();
                SE.Debug.Assert(!previous.IsFocused, "Previous control is still focused");
            }

            if (m_FocusedControl != null)
            {
                m_FocusedControl.OnGetFocus();
                SE.Debug.Assert(m_FocusedControl.IsFocused, "Current control is not focused");
            }

            // Update flags
            UpdateContainsFocus();

            return true;
        }

        /// <inheritdoc />
        public override CursorType Cursor
        {
            get
            {
                return m_Window.Cursor;
            }
            set
            {
                m_Window.Cursor = value;
            }
        }
    }
}
