using System;

namespace SE
{
    /// <summary>
    /// Managed GUI host extensions for the generated native window wrapper.
    /// </summary>
    public sealed partial class Window
    {
        private readonly GUI.WindowRootControl _gui;

        /// <summary>
        /// Called when the native window asks whether a client position belongs to a non-client region.
        /// The hit result uses the native <c>WindowHitCodes</c> value until that enum is exposed by the bindings.
        /// </summary>
        public delegate void HitTestDelegate(Float2 position, ref int result, ref bool handled);

        /// <summary>
        /// Called when the native window handles a left mouse button hit in a non-client region.
        /// </summary>
        public delegate void LeftButtonHitDelegate(int hit, ref bool handled);

        /// <summary>
        /// Called before the native window closes. Set <paramref name="cancel"/> to cancel a normal close.
        /// </summary>
        public delegate void ClosingDelegate(ClosingReason reason, ref bool cancel);

        /// <summary>
        /// Called for a native drag operation. Return a native <c>DragDropEffect</c> value.
        /// </summary>
        public delegate int DragDropDelegate(Float2 position, bool isText, string[] values);

        public event Action Shown;
        public event Action<Float2> Resized;
        public event Action<float> Updating;
        public event Action Drawing;
        public event Action<char> CharInput;
        public event Action<int> KeyDown;
        public event Action<int> KeyUp;
        public event Action<Float2, int> MouseDown;
        public event Action<Float2, int> MouseUp;
        public event Action<Float2, int> MouseDoubleClick;
        public event Action<Float2, float> MouseWheel;
        public event Action<Float2> MouseMove;
        public event Action MouseLeave;
        public event Action<Float2, int> TouchDown;
        public event Action<Float2, int> TouchMove;
        public event Action<Float2, int> TouchUp;
        public event Action GotFocus;
        public event Action LostFocus;
        public event Action DragLeave;
        public event HitTestDelegate HitTest;
        public event LeftButtonHitDelegate LeftButtonHit;
        public event ClosingDelegate Closing;
        public event Action Closed;
        public event DragDropDelegate DragEnter;
        public event DragDropDelegate DragOver;
        public event DragDropDelegate DragDrop;

        public Window()
        {
            _gui = new GUI.WindowRootControl(this);
        }

        /// <summary>
        /// Gets the managed GUI root owned by this window.
        /// The tree becomes active when its native host initializes the managed backend.
        /// </summary>
        public GUI.WindowRootControl GUI => _gui;

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
            _gui.Initialize(logicalSize, dpiScale);
        }

        // WindowBase invokes the Internal_On* methods. GraphicWindow owns the parallel
        // Internal_OnGui* path so these notifications never dispatch the GUI tree twice.
        internal void Internal_OnShow()
        {
            Shown?.Invoke();
        }

        internal void Internal_OnResize(int width, int height)
        {
            Resized?.Invoke(new Float2(width, height));
        }

        internal void Internal_OnUpdate(float deltaTime)
        {
            Updating?.Invoke(deltaTime);
        }

        internal void Internal_OnDraw()
        {
            Drawing?.Invoke();
        }

        internal void Internal_OnCharInput(char character)
        {
            CharInput?.Invoke(character);
        }

        internal void Internal_OnKeyDown(int key)
        {
            KeyDown?.Invoke(key);
        }

        internal void Internal_OnKeyUp(int key)
        {
            KeyUp?.Invoke(key);
        }

        internal void Internal_OnMouseDown(Float2 position, int button)
        {
            MouseDown?.Invoke(position, button);
        }

        internal void Internal_OnMouseUp(Float2 position, int button)
        {
            MouseUp?.Invoke(position, button);
        }

        internal void Internal_OnMouseDoubleClick(Float2 position, int button)
        {
            MouseDoubleClick?.Invoke(position, button);
        }

        internal void Internal_OnMouseWheel(Float2 position, float delta)
        {
            MouseWheel?.Invoke(position, delta);
        }

        internal void Internal_OnMouseMove(Float2 position)
        {
            MouseMove?.Invoke(position);
        }

        internal void Internal_OnMouseLeave()
        {
            MouseLeave?.Invoke();
        }

        internal void Internal_OnTouchDown(Float2 position, int pointerId)
        {
            TouchDown?.Invoke(position, pointerId);
        }

        internal void Internal_OnTouchMove(Float2 position, int pointerId)
        {
            TouchMove?.Invoke(position, pointerId);
        }

        internal void Internal_OnTouchUp(Float2 position, int pointerId)
        {
            TouchUp?.Invoke(position, pointerId);
        }

        internal int Internal_OnDragEnter(Float2 position, bool isText, string[] values)
        {
            return DragEnter?.Invoke(position, isText, values) ?? 0;
        }

        internal int Internal_OnDragOver(Float2 position, bool isText, string[] values)
        {
            return DragOver?.Invoke(position, isText, values) ?? 0;
        }

        internal int Internal_OnDragDrop(Float2 position, bool isText, string[] values)
        {
            return DragDrop?.Invoke(position, isText, values) ?? 0;
        }

        internal void Internal_OnDragLeave()
        {
            DragLeave?.Invoke();
        }

        internal void Internal_OnHitTest(Float2 position, ref int result, ref bool handled)
        {
            HitTest?.Invoke(position, ref result, ref handled);
        }

        internal void Internal_OnLeftButtonHit(int hit, ref bool handled)
        {
            LeftButtonHit?.Invoke(hit, ref handled);
        }

        internal void Internal_OnClosing(ClosingReason reason, ref bool cancel)
        {
            Closing?.Invoke(reason, ref cancel);
        }

        internal void Internal_OnClosed()
        {
            Closed?.Invoke();

            Shown = null;
            Resized = null;
            Updating = null;
            Drawing = null;
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
            GotFocus = null;
            LostFocus = null;
            DragLeave = null;
            HitTest = null;
            LeftButtonHit = null;
            Closing = null;
            Closed = null;
            DragEnter = null;
            DragOver = null;
            DragDrop = null;
        }

        internal void Internal_OnGotFocus()
        {
            GotFocus?.Invoke();
        }

        internal void Internal_OnLostFocus()
        {
            LostFocus?.Invoke();
        }

        internal void Internal_OnGuiShown()
        {
            if (_gui.IsInitialized)
                _gui.PerformLayout(true);
        }

        internal void Internal_OnGuiUpdate(float deltaTime)
        {
            if (_gui.IsInitialized)
                _gui.Update(deltaTime);
        }

        internal void Internal_OnGuiDraw()
        {
            if (_gui.IsInitialized)
                _gui.Draw();
        }

        internal void Internal_OnGuiResize(Float2 logicalSize, float dpiScale)
        {
            _gui.Resize(logicalSize, dpiScale);
        }

        internal void Internal_OnGuiCharInput(char character)
        {
            if (_gui.IsInitialized)
                _gui.OnCharInput(character);
        }

        internal void Internal_OnGuiKeyDown(int key)
        {
            if (_gui.IsInitialized)
                _gui.OnKeyDown(key);
        }

        internal void Internal_OnGuiKeyUp(int key)
        {
            if (_gui.IsInitialized)
                _gui.OnKeyUp(key);
        }

        internal void Internal_OnGuiMouseDown(Float2 logicalPosition, int button)
        {
            if (_gui.IsInitialized)
                _gui.OnMouseDown(logicalPosition, button);
        }

        internal void Internal_OnGuiMouseUp(Float2 logicalPosition, int button)
        {
            if (_gui.IsInitialized)
                _gui.OnMouseUp(logicalPosition, button);
        }

        internal void Internal_OnGuiMouseDoubleClick(Float2 logicalPosition, int button)
        {
            if (_gui.IsInitialized)
                _gui.OnMouseDoubleClick(logicalPosition, button);
        }

        internal void Internal_OnGuiMouseWheel(Float2 logicalPosition, float delta)
        {
            if (_gui.IsInitialized)
                _gui.OnMouseWheel(logicalPosition, delta);
        }

        internal void Internal_OnGuiMouseMove(Float2 logicalPosition)
        {
            if (_gui.IsInitialized)
                _gui.OnMouseMove(logicalPosition);
        }

        internal void Internal_OnGuiMouseLeave()
        {
            if (_gui.IsInitialized)
                _gui.OnMouseLeave();
        }

        internal void Internal_OnGuiTouchDown(Float2 logicalPosition, int pointerIndex)
        {
            if (_gui.IsInitialized)
                _gui.OnTouchDown(logicalPosition, pointerIndex);
        }

        internal void Internal_OnGuiTouchMove(Float2 logicalPosition, int pointerIndex)
        {
            if (_gui.IsInitialized)
                _gui.OnTouchMove(logicalPosition, pointerIndex);
        }

        internal void Internal_OnGuiTouchUp(Float2 logicalPosition, int pointerIndex)
        {
            if (_gui.IsInitialized)
                _gui.OnTouchUp(logicalPosition, pointerIndex);
        }

        internal void Internal_OnGuiFocusChanged(bool focused)
        {
            if (_gui.IsInitialized)
                _gui.OnWindowFocusChanged(focused);
        }

        internal int Internal_OnGuiDragEnter(Float2 logicalPosition, bool isText, string[] values)
        {
            return _gui.IsInitialized ? (int)_gui.OnDragEnter(logicalPosition, CreateDragData(isText, values)) : 0;
        }

        internal int Internal_OnGuiDragMove(Float2 logicalPosition, bool isText, string[] values)
        {
            return _gui.IsInitialized ? (int)_gui.OnDragMove(logicalPosition, CreateDragData(isText, values)) : 0;
        }

        internal int Internal_OnGuiDragDrop(Float2 logicalPosition, bool isText, string[] values)
        {
            return _gui.IsInitialized ? (int)_gui.OnDragDrop(logicalPosition, CreateDragData(isText, values)) : 0;
        }

        internal void Internal_OnGuiDragLeave()
        {
            if (_gui.IsInitialized)
                _gui.OnDragLeave();
        }

        internal void Internal_DisposeGui()
        {
            _gui.Dispose();
        }

        private static GUI.DragData CreateDragData(bool isText, string[] values)
        {
            return isText
                ? new GUI.DragDataText(values.Length > 0 ? values[0] : string.Empty)
                : new GUI.DragDataFiles(values);
        }
    }
}
