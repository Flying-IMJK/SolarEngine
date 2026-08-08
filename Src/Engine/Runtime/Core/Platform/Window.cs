using System;
using SE.GUI;

namespace SE
{
    /// <summary>
    /// Managed GUI host extensions for the generated native window wrapper.
    /// </summary>
    public sealed partial class Window
    {
        private readonly GUI.WindowRootControl m_GUI;

        /// <summary>
        /// Called when the native window asks whether a client position belongs to a non-client region.
        /// The hit result uses the native <c>WindowHitCodes</c> value until that enum is exposed by the bindings.
        /// </summary>
        public delegate WindowHitCodes HitTestDelegate(ref Float2 mouse);

        /// <summary>
        /// Window closing delegate.
        /// </summary>
        /// <param name="reason">The closing reason.</param>
        /// <param name="cancel">If set to <c>true</c> operation will be cancelled, otherwise window will be closed.</param>
        public delegate void ClosingDelegate(ClosingReason reason, ref bool cancel);
        

        /// <summary>
        /// Perform input character action.
        /// </summary>
        /// <param name="c">The input character.</param>
        public delegate void CharDelegate(char c);
        
        /// <summary>
        /// Perform keyboard action.
        /// </summary>
        /// <param name="key">The key.</param>
        public delegate void KeyboardDelegate(KeyboardKeys key);
        
        /// <summary>
        /// Perform mouse buttons action.
        /// </summary>
        /// <param name="mouse">The mouse position.</param>
        /// <param name="button">The mouse buttons state.</param>
        /// <param name="handled">The flag that indicated that event has been handled by the custom code and should not be passed further. By default it is set to false.</param>
        public delegate void MouseButtonDelegate(ref Float2 mouse, MouseButton button, ref bool handled);
        
        /// <summary>
        /// Perform mouse wheel action.
        /// </summary>
        /// <param name="mouse">The mouse position.</param>
        /// <param name="delta">The mouse wheel move delta (can be positive or negative; normalized to [-1;1] range).</param>
        /// <param name="handled">The flag that indicated that event has been handled by the custom code and should not be passed further. By default it is set to false.</param>
        public delegate void MouseWheelDelegate(ref Float2 mouse, float delta, ref bool handled);
        
        
        /// <summary>
        /// Perform mouse move action.
        /// </summary>
        /// <param name="mouse">The mouse position.</param>
        public delegate void MouseMoveDelegate(ref Float2 mouse);
        
        /// <summary>
        /// Perform touch action.
        /// </summary>
        /// <param name="pointerPosition">The touch pointer position.</param>
        /// <param name="pointerId">The touch pointer identifier. Stable for the whole touch gesture/interaction.</param>
        /// <param name="handled">The flag that indicated that event has been handled by the custom code and should not be passed further. By default it is set to false.</param>
        public delegate void TouchDelegate(ref Float2 pointerPosition, int pointerId, ref bool handled);

        public event CharDelegate CharInput;
        public event KeyboardDelegate KeyDown;
        public event KeyboardDelegate KeyUp;
        public event MouseButtonDelegate MouseDown;
        public event MouseButtonDelegate MouseUp;
        public event MouseButtonDelegate MouseDoubleClick;
        public event MouseWheelDelegate MouseWheel;
        public event MouseMoveDelegate MouseMove;
        public event Action MouseLeave;
        public event TouchDelegate TouchDown;
        public event TouchDelegate TouchMove;
        public event TouchDelegate TouchUp;
        public event Action GetFocus;
        public event Action LostFocus;
        
        public HitTestDelegate HitTest;
        
        public Func<WindowHitCodes, bool> LeftButtonHit;
        public event ClosingDelegate Closing;
        public event Action Closed;

        public Window()
        {
            m_GUI = new GUI.WindowRootControl(this);
        }

        /// <summary>
        /// Gets the managed GUI root owned by this window.
        /// The tree becomes active when its native host initializes the managed backend.
        /// </summary>
        public GUI.WindowRootControl GUI => m_GUI;
        

        /// <summary>
        /// Creates a managed GUI window with the native platform defaults.
        /// </summary>
        public static Window CreateManaged()
        {
            return Create(CreateDefaultSettings());
        }

        /// <summary>
        /// Creates a managed GUI window with the supplied title and client size.
        /// </summary>
        public static Window CreateManaged(string title, Float2 clientSize)
        {
            var settings = CreateDefaultSettings();
            settings.Title = title;
            settings.Size = clientSize;
            return Create(settings);
        }

        /// <summary>
        /// Gets or sets whether the native window is visible.
        /// </summary>
        public bool Visible
        {
            get => IsVisible();
            set => SetIsVisible(value);
        }

        /// <summary>
        /// Gets or sets the client-area size in physical pixels.
        /// </summary>
        public Float2 ClientSize
        {
            get => GetClientSize();
            set
            {
                var clientSize = value;
                SetClientSize(ref clientSize);
            }
        }

        /// <summary>
        /// Gets or sets the window title.
        /// </summary>
        public string Title
        {
            get => GetTitle();
            set => SetTitle(value);
        }

        /// <summary>
        /// Gets the scale that converts physical window coordinates to logical GUI coordinates.
        /// </summary>
        public float DpiScale => GetDpiScale();

        internal void Internal_InitializeGui(Float2 logicalSize, float dpiScale)
        {
            m_GUI.Initialize(logicalSize, dpiScale);
        }

        // WindowBase invokes the Internal_On* methods. GraphicWindow owns the parallel
        // Internal_OnGui* path so these notifications never dispatch the GUI tree twice.
        internal void Internal_OnShow()
        {
            GUI.UnlockChildrenRecursive();
            GUI.PerformLayout();
        }

        internal void Internal_OnResize(int width, int height)
        {
            GUI.Size = new Float2(width / DpiScale, height / DpiScale);
        }

        internal void Internal_OnUpdate(float deltaTime)
        {
            GUI.Update(deltaTime);
        }

        internal void Internal_OnDraw()
        {
            Matrix3x3.Scaling(DpiScale, out var scale);
            Render2D.PushTransform(ref scale);
            GUI.Draw();
            Render2D.PopTransform();
        }

        internal void Internal_OnCharInput(char character)
        {
            CharInput?.Invoke(character);
            GUI.OnCharInput(character);
        }

        internal void Internal_OnKeyDown(KeyboardKeys key)
        {
            KeyDown?.Invoke(key);
            GUI.OnKeyDown(key);
        }

        internal void Internal_OnKeyUp(KeyboardKeys key)
        {
            KeyUp?.Invoke(key);
            GUI.OnKeyUp(key);
        }

        internal void Internal_OnMouseDown(Float2 position, MouseButton button)
        {
            var pos = position / DpiScale;

            bool handled = false;
            MouseDown?.Invoke(ref pos, button, ref handled);
            if (handled)
            {
                return;
            }

            GUI.OnMouseDown(pos, button);
        }

        internal void Internal_OnMouseUp(Float2 position, MouseButton button)
        {
            var pos = position / DpiScale;

            bool handled = false;
            MouseUp?.Invoke(ref pos, button, ref handled);
            if (handled)
            {
                return;
            }

            GUI.OnMouseUp(pos, button);
        }

        internal void Internal_OnMouseDoubleClick(Float2 position, MouseButton button)
        {
            var pos = position / DpiScale;

            bool handled = false;
            MouseDoubleClick?.Invoke(ref pos, button, ref handled);
            if (handled)
            {
                return;
            }

            GUI.OnMouseDoubleClick(pos, button);
        }

        internal void Internal_OnMouseWheel(Float2 position, float delta)
        {
            var pos = position / DpiScale;

            bool handled = false;
            MouseWheel?.Invoke(ref pos, delta, ref handled);
            if (handled)
                return;

            GUI.OnMouseWheel(pos, delta);
        }

        internal void Internal_OnMouseMove(Float2 position)
        {
            var pos = position / DpiScale;

            MouseMove?.Invoke(ref pos);
            GUI.OnMouseMove(pos);
        }

        internal void Internal_OnMouseLeave()
        {
            MouseLeave?.Invoke();
            GUI.OnMouseLeave();
        }

        internal void Internal_OnTouchDown(Float2 position, int pointerId)
        {
            var pos = position / DpiScale;

            bool handled = false;
            TouchDown?.Invoke(ref pos, pointerId, ref handled);
            if (handled)
                return;

            GUI.OnTouchDown(pos, pointerId);
        }

        internal void Internal_OnTouchMove(Float2 position, int pointerId)
        {
            var pos = position / DpiScale;

            bool handled = false;
            TouchMove?.Invoke(ref pos, pointerId, ref handled);
            if (handled)
                return;

            GUI.OnTouchMove(pos, pointerId);
        }

        internal void Internal_OnTouchUp(Float2 position, int pointerId)
        {
            var pos = position / DpiScale;

            bool handled = false;
            TouchUp?.Invoke(ref pos, pointerId, ref handled);
            if (handled)
                return;

            GUI.OnTouchUp(pos, pointerId);
        }

        internal DragDropEffect Internal_OnDragEnter(Float2 position, bool isText, string[] values)
        {
            DragData dragData;
            if (isText)
            {
                dragData = new DragDataText(values[0]);
            }
            else
            {
                dragData = new DragDataFiles(values);
            }

            var pos = position / DpiScale;
            return GUI.OnDragEnter(ref pos, dragData);
        }

        internal DragDropEffect Internal_OnDragOver(Float2 position, bool isText, string[] values)
        {
            DragData dragData;
            if (isText)
            {
                dragData = new DragDataText(values[0]);
            }
            else
            {
                dragData = new DragDataFiles(values);
            }

            var pos = position / DpiScale;
            return GUI.OnDragMove(ref pos, dragData);
        }

        internal DragDropEffect Internal_OnDragDrop(Float2 position, bool isText, string[] values)
        {
            DragData dragData;
            if (isText)
            {
                dragData = new DragDataText(values[0]);
            }
            else
            {
                dragData = new DragDataFiles(values);
            }

            var pos = position / DpiScale;
            return GUI.OnDragDrop(ref pos, dragData);
        }

        internal void Internal_OnDragLeave()
        {
            GUI.OnDragLeave();
        }

        internal void Internal_OnHitTest(Float2 position, ref WindowHitCodes result, ref bool handled)
        {
            if (HitTest != null)
            {
                var pos = position / DpiScale;
                result = HitTest(ref pos);
                handled = true;
            }
        }

        internal void Internal_OnLeftButtonHit(WindowHitCodes hit, ref bool handled)
        {
            if (LeftButtonHit != null)
            {
                handled = LeftButtonHit(hit);
            }
        }

        internal void Internal_OnClosing(ClosingReason reason, ref bool cancel)
        {
            Closing?.Invoke(reason, ref cancel);
        }

        internal void Internal_OnGotFocus()
        {
            GetFocus?.Invoke();
            GUI.OnGetFocus();
        }

        internal void Internal_OnLostFocus()
        {
            LostFocus?.Invoke();
            GUI.OnLostFocus();
        }
        
        internal void Internal_OnClosed()
        {
            Closed?.Invoke();
            
            GUI.Dispose();
            
            CharInput = null;
            KeyDown = null;
            KeyUp = null;
            MouseDown = null;
            MouseUp = null;
            MouseDoubleClick = null;
            MouseWheel = null;
            MouseMove = null;
            MouseLeave = null;
            TouchDown = null;
            TouchMove = null;
            TouchUp = null;
            GetFocus = null;
            LostFocus = null;
            HitTest = null;
            LeftButtonHit = null;
            Closing = null;
            Closed = null;
        }
    }
}
