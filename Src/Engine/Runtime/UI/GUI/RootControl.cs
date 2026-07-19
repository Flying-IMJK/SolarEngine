using System;

namespace SE.GUI
{
    /// <summary>
    /// Root of a managed GUI tree. It owns focus, pointer capture and event routing.
    /// </summary>
    public class RootControl : ContainerControl
    {
        private Control? _focusedControl;
        private Control? _trackingControl;
        private Control? _mouseOverControl;
        private DragData? _dragData;
        private readonly Tooltip _tooltip;

        public RootControl()
        {
            SetRootCore(this);
            _tooltip = new Tooltip();
        }

        public RootControl(Rectangle bounds)
            : base(bounds)
        {
            SetRootCore(this);
            _tooltip = new Tooltip();
        }

        /// <summary>
        /// Gets the game GUI root when one has been registered by the host.
        /// </summary>
        public static ContainerControl? GameRoot { get; private set; }

        /// <summary>
        /// Gets or sets the currently focused control.
        /// </summary>
        public Control? FocusedControl
        {
            get => _focusedControl;
            set => Focus(value);
        }

        /// <summary>
        /// Gets the currently captured pointer target.
        /// </summary>
        public Control? TrackingControl => _trackingControl;

        /// <summary>
        /// Gets the pointer position in root logical coordinates.
        /// </summary>
        public Float2 MousePosition { get; private set; }

        /// <summary>
        /// Gets the current DPI scale supplied by the owning window.
        /// </summary>
        public float DpiScale { get; private set; } = 1.0f;

        /// <summary>
        /// Gets the managed tooltip service for this root.
        /// </summary>
        public Tooltip Tooltip => _tooltip;

        /// <summary>
        /// Registers the game GUI root.
        /// </summary>
        public static void SetGameRoot(ContainerControl? value)
        {
            GameRoot = value;
        }

        /// <summary>
        /// Updates the root's DPI scale and logical bounds.
        /// </summary>
        public void SetWindowMetrics(Float2 logicalSize, float dpiScale)
        {
            DpiScale = dpiScale > 0.0f ? dpiScale : 1.0f;
            SetBounds(0.0f, 0.0f, logicalSize.X, logicalSize.Y);
            PerformLayout(true);
        }

        /// <summary>
        /// Starts pointer capture for a control in this root tree.
        /// </summary>
        public void StartTrackingMouse(Control control)
        {
            ArgumentNullException.ThrowIfNull(control);
            if (!ReferenceEquals(control.Root, this))
                throw new InvalidOperationException("Only controls in this root can capture its pointer.");

            _trackingControl = control;
            if (control.AutoFocus)
                Focus(control);
        }

        /// <summary>
        /// Ends pointer capture.
        /// </summary>
        public void EndTrackingMouse()
        {
            _trackingControl = null;
        }

        /// <summary>
        /// Focuses a control in this root tree, or clears focus.
        /// </summary>
        public bool Focus(Control? control)
        {
            if (ReferenceEquals(_focusedControl, control))
                return false;
            if (control != null && (!ReferenceEquals(control.Root, this) || !control.VisibleInHierarchy || !control.EnabledInHierarchy))
                return false;

            Control? previous = _focusedControl;
            _focusedControl = control;
            previous?.SetFocused(false);
            control?.SetFocused(true);
            return true;
        }

        public override bool OnCharInput(char character)
        {
            return _focusedControl?.OnCharInput(character) ?? false;
        }

        public override bool OnKeyDown(int key)
        {
            return _focusedControl?.OnKeyDown(key) ?? false;
        }

        public override bool OnKeyUp(int key)
        {
            return _focusedControl?.OnKeyUp(key) ?? false;
        }

        public override bool OnMouseDown(Float2 location, int button)
        {
            MousePosition = location;
            Control? control = _trackingControl ?? HitTest(location);
            UpdateMouseOver(location);
            if (control == null || ReferenceEquals(control, this))
                return false;

            if (control.AutoFocus)
                Focus(control);
            return control.OnMouseDown(control.PointFromRoot(location), button);
        }

        public override bool OnMouseUp(Float2 location, int button)
        {
            MousePosition = location;
            Control? control = _trackingControl ?? HitTest(location);
            UpdateMouseOver(location);
            return control == null || ReferenceEquals(control, this) ? false : control.OnMouseUp(control.PointFromRoot(location), button);
        }

        public override bool OnMouseDoubleClick(Float2 location, int button)
        {
            MousePosition = location;
            Control? control = _trackingControl ?? HitTest(location);
            UpdateMouseOver(location);
            return control == null || ReferenceEquals(control, this) ? false : control.OnMouseDoubleClick(control.PointFromRoot(location), button);
        }

        public override bool OnMouseWheel(Float2 location, float delta)
        {
            MousePosition = location;
            Control? control = _trackingControl ?? HitTest(location);
            UpdateMouseOver(location);
            return control == null || ReferenceEquals(control, this) ? false : control.OnMouseWheel(control.PointFromRoot(location), delta);
        }

        public override void OnMouseMove(Float2 location)
        {
            MousePosition = location;
            Control? control = _trackingControl ?? HitTest(location);
            UpdateMouseOver(location);
            if (control != null && !ReferenceEquals(control, this))
                control.OnMouseMove(control.PointFromRoot(location));
        }

        public override void OnMouseLeave()
        {
            if (_mouseOverControl != null)
                _tooltip.OnMouseLeaveControl(_mouseOverControl);
            _mouseOverControl?.SetMouseOver(false);
            _mouseOverControl = null;
        }

        public override bool OnTouchDown(Float2 location, int pointerIndex)
        {
            Control? control = HitTest(location);
            if (control?.AutoFocus == true)
                Focus(control);
            return control == null || ReferenceEquals(control, this) ? false : control.OnTouchDown(control.PointFromRoot(location), pointerIndex);
        }

        public override void OnTouchMove(Float2 location, int pointerIndex)
        {
            Control? control = HitTest(location);
            if (control != null && !ReferenceEquals(control, this))
                control.OnTouchMove(control.PointFromRoot(location), pointerIndex);
        }

        public override bool OnTouchUp(Float2 location, int pointerIndex)
        {
            Control? control = HitTest(location);
            return control == null || ReferenceEquals(control, this) ? false : control.OnTouchUp(control.PointFromRoot(location), pointerIndex);
        }

        public override DragDropEffect OnDragEnter(Float2 location, DragData data)
        {
            ArgumentNullException.ThrowIfNull(data);
            MousePosition = location;
            _dragData = data;
            Control? control = _trackingControl ?? HitTest(location);
            UpdateMouseOver(location);
            return control == null || ReferenceEquals(control, this) ? DragDropEffect.None : control.OnDragEnter(control.PointFromRoot(location), data);
        }

        public override DragDropEffect OnDragMove(Float2 location, DragData data)
        {
            ArgumentNullException.ThrowIfNull(data);
            MousePosition = location;
            _dragData = data;
            Control? control = _trackingControl ?? HitTest(location);
            UpdateMouseOver(location);
            return control == null || ReferenceEquals(control, this) ? DragDropEffect.None : control.OnDragMove(control.PointFromRoot(location), data);
        }

        public override DragDropEffect OnDragDrop(Float2 location, DragData data)
        {
            ArgumentNullException.ThrowIfNull(data);
            MousePosition = location;
            Control? control = _trackingControl ?? HitTest(location);
            DragDropEffect result = control == null || ReferenceEquals(control, this) ? DragDropEffect.None : control.OnDragDrop(control.PointFromRoot(location), data);
            _dragData = null;
            return result;
        }

        public override void OnDragLeave()
        {
            _dragData = null;
            _mouseOverControl?.OnDragLeave();
        }

        public override void ClearState()
        {
            _trackingControl = null;
            _dragData = null;
            _tooltip.Hide();
            _mouseOverControl?.SetMouseOver(false);
            _mouseOverControl = null;
            Focus(null);
            base.ClearState();
        }

        protected override void OnDispose()
        {
            if (ReferenceEquals(GameRoot, this))
                GameRoot = null;
            _tooltip.Dispose();
            base.OnDispose();
        }

        public override void Update(float deltaTime)
        {
            base.Update(deltaTime);
            if (!ReferenceEquals(_tooltip.Parent, this))
                _tooltip.Update(deltaTime);
        }

        private void UpdateMouseOver(Float2 location)
        {
            Control? next = HitTest(location);
            if (ReferenceEquals(next, _mouseOverControl))
                return;

            if (_mouseOverControl != null)
                _tooltip.OnMouseLeaveControl(_mouseOverControl);
            _mouseOverControl?.SetMouseOver(false);
            _mouseOverControl = next;
            _mouseOverControl?.SetMouseOver(true);
            if (_mouseOverControl != null && !ReferenceEquals(_mouseOverControl, this))
                _tooltip.OnMouseEnterControl(_mouseOverControl);
        }
    }
}
