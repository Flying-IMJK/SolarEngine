using System;

namespace SE.GUI
{
    /// <summary>
    /// Root of a managed GUI tree. It owns focus, pointer capture and event routing.
    /// </summary>
    public class RootControl : ContainerControl
    {
        private Control? m_FocusedControl;
        private Control? m_TrackingControl;
        private Control? m_MouseOverControl;
        private DragData? m_DragData;
        private readonly Tooltip m_Tooltip;

        public RootControl()
        {
            SetRootCore(this);
            m_Tooltip = new Tooltip();
        }

        public RootControl(Rectangle bounds)
            : base(bounds)
        {
            SetRootCore(this);
            m_Tooltip = new Tooltip();
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
            get => m_FocusedControl;
            set => Focus(value);
        }

        /// <summary>
        /// Gets the currently captured pointer target.
        /// </summary>
        public Control? TrackingControl => m_TrackingControl;

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
        public Tooltip Tooltip => m_Tooltip;

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

            m_TrackingControl = control;
            if (control.AutoFocus)
                Focus(control);
        }

        /// <summary>
        /// Ends pointer capture.
        /// </summary>
        public void EndTrackingMouse()
        {
            m_TrackingControl = null;
        }

        /// <summary>
        /// Focuses a control in this root tree, or clears focus.
        /// </summary>
        public bool Focus(Control? control)
        {
            if (ReferenceEquals(m_FocusedControl, control))
                return false;
            if (control != null && (!ReferenceEquals(control.Root, this) || !control.VisibleInHierarchy || !control.EnabledInHierarchy))
                return false;

            Control? previous = m_FocusedControl;
            m_FocusedControl = control;
            previous?.SetFocused(false);
            control?.SetFocused(true);
            return true;
        }

        /// <summary>
        /// Gets the key state for this root.
        /// </summary>
        public virtual bool GetKey(KeyboardKeys key)
        {
            _ = key;
            return false;
        }

        /// <summary>
        /// Gets the key down state for this root.
        /// </summary>
        public virtual bool GetKeyDown(KeyboardKeys key)
        {
            _ = key;
            return false;
        }

        /// <summary>
        /// Gets the key up state for this root.
        /// </summary>
        public virtual bool GetKeyUp(KeyboardKeys key)
        {
            _ = key;
            return false;
        }

        public override bool OnCharInput(char character)
        {
            return m_FocusedControl?.OnCharInput(character) ?? false;
        }

        public override bool OnKeyDown(KeyboardKeys key)
        {
            return m_FocusedControl?.OnKeyDown(key) ?? false;
        }

        public override bool OnKeyUp(KeyboardKeys key)
        {
            return m_FocusedControl?.OnKeyUp(key) ?? false;
        }

        public override bool OnMouseDown(Float2 location, MouseButton button)
        {
            MousePosition = location;
            Control? control = m_TrackingControl ?? HitTest(location);
            UpdateMouseOver(location);
            if (control == null || ReferenceEquals(control, this))
                return false;

            if (control.AutoFocus)
                Focus(control);
            return control.OnMouseDown(control.PointFromRoot(location), button);
        }

        public override bool OnMouseUp(Float2 location, MouseButton button)
        {
            MousePosition = location;
            Control? control = m_TrackingControl ?? HitTest(location);
            UpdateMouseOver(location);
            return control == null || ReferenceEquals(control, this) ? false : control.OnMouseUp(control.PointFromRoot(location), button);
        }

        public override bool OnMouseDoubleClick(Float2 location, MouseButton button)
        {
            MousePosition = location;
            Control? control = m_TrackingControl ?? HitTest(location);
            UpdateMouseOver(location);
            return control == null || ReferenceEquals(control, this) ? false : control.OnMouseDoubleClick(control.PointFromRoot(location), button);
        }

        public override bool OnMouseWheel(Float2 location, float delta)
        {
            MousePosition = location;
            Control? control = m_TrackingControl ?? HitTest(location);
            UpdateMouseOver(location);
            return control == null || ReferenceEquals(control, this) ? false : control.OnMouseWheel(control.PointFromRoot(location), delta);
        }

        public override void OnMouseMove(Float2 location)
        {
            MousePosition = location;
            Control? control = m_TrackingControl ?? HitTest(location);
            UpdateMouseOver(location);
            if (control != null && !ReferenceEquals(control, this))
                control.OnMouseMove(control.PointFromRoot(location));
        }

        public override void OnMouseLeave()
        {
            if (m_MouseOverControl != null)
                m_Tooltip.OnMouseLeaveControl(m_MouseOverControl);
            m_MouseOverControl?.SetMouseOver(false);
            m_MouseOverControl = null;
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

        public override DragDropEffect OnDragEnter(ref Float2 location, DragData data)
        {
            ArgumentNullException.ThrowIfNull(data);
            MousePosition = location;
            m_DragData = data;
            Control? control = m_TrackingControl ?? HitTest(location);
            UpdateMouseOver(location);

            if (control == null || ReferenceEquals(control, this))
            {
                return DragDropEffect.None;
            }

            Float2 pos = control.PointFromRoot(location);
            return control.OnDragEnter(ref pos, data);
        }

        public override DragDropEffect OnDragMove(ref Float2 location, DragData data)
        {
            ArgumentNullException.ThrowIfNull(data);
            MousePosition = location;
            m_DragData = data;
            Control? control = m_TrackingControl ?? HitTest(location);
            UpdateMouseOver(location);

            if (control == null || ReferenceEquals(control, this))
            {
                return DragDropEffect.None;
            }

            Float2 pos = control.PointFromRoot(location);
            return control.OnDragMove(ref pos, data);
        }

        public override DragDropEffect OnDragDrop(ref Float2 location, DragData data)
        {
            ArgumentNullException.ThrowIfNull(data);
            MousePosition = location;
            Control? control = m_TrackingControl ?? HitTest(location);
            DragDropEffect result = DragDropEffect.None;
            if (!(control == null || ReferenceEquals(control, this)))
            {
                Float2 pos = control.PointFromRoot(location);
                result = control.OnDragDrop(ref pos, data);
            }

            m_DragData = null;
            return result;
        }

        public override void OnDragLeave()
        {
            m_DragData = null;
            m_MouseOverControl?.OnDragLeave();
        }

        public override void ClearState()
        {
            m_TrackingControl = null;
            m_DragData = null;
            m_Tooltip.Hide();
            m_MouseOverControl?.SetMouseOver(false);
            m_MouseOverControl = null;
            Focus(null);
            base.ClearState();
        }

        protected override void OnDispose()
        {
            if (ReferenceEquals(GameRoot, this))
                GameRoot = null;
            m_Tooltip.Dispose();
            base.OnDispose();
        }

        public override void Update(float deltaTime)
        {
            base.Update(deltaTime);
            if (!ReferenceEquals(m_Tooltip.Parent, this))
                m_Tooltip.Update(deltaTime);
        }

        private void UpdateMouseOver(Float2 location)
        {
            Control? next = HitTest(location);
            if (ReferenceEquals(next, m_MouseOverControl))
                return;

            if (m_MouseOverControl != null)
                m_Tooltip.OnMouseLeaveControl(m_MouseOverControl);
            m_MouseOverControl?.SetMouseOver(false);
            m_MouseOverControl = next;
            m_MouseOverControl?.SetMouseOver(true);
            if (m_MouseOverControl != null && !ReferenceEquals(m_MouseOverControl, this))
                m_Tooltip.OnMouseEnterControl(m_MouseOverControl);
        }
    }
}
