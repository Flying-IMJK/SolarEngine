using System;
using System.Collections.Generic;

namespace SE.GUI
{
    /// <summary>
    /// Base class for all managed GUI controls.
    /// </summary>
    public class Control : IDisposable
    {
        private ContainerControl? m_Parent;
        private RootControl? m_Root;
        private Rectangle m_Bounds;
        private bool m_Visible = true;
        private bool m_Enabled = true;
        private bool m_IsDisposing;
        private bool m_IsDisposed;
        private bool m_IsMouseOver;
        private bool m_IsDragOver;
        private bool m_IsFocused;

        /// <summary>
        /// Initializes a new instance of the <see cref="Control"/> class.
        /// </summary>
        public Control() : this(Rectangle.Empty)
        {
        }


        /// <summary>
        /// Initializes a new instance of the <see cref="Control"/> class.
        /// </summary>
        /// <param name="x">X coordinate</param>
        /// <param name="y">Y coordinate</param>
        /// <param name="width">Width</param>
        /// <param name="height">Height</param>
        public Control(float x, float y, float width, float height)
            : this(new Rectangle(x, y, width, height))
        {
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="Control"/> class.
        /// </summary>
        /// <param name="location">Upper left corner location.</param>
        /// <param name="size">Bounds size.</param>
        public Control(Float2 location, Float2 size)
            : this(new Rectangle(location, size))
        {
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="Control"/> class.
        /// </summary>
        public Control(Rectangle bounds)
        {
            m_Bounds = bounds;
        }

        /// <summary>
        /// Raised after the control location changes.
        /// </summary>
        public event Action<Control>? LocationChanged;

        /// <summary>
        /// Raised after the control size changes.
        /// </summary>
        public event Action<Control>? SizeChanged;

        /// <summary>
        /// Raised after the parent changes.
        /// </summary>
        public event Action<Control>? ParentChanged;

        /// <summary>
        /// Raised after the visibility changes.
        /// </summary>
        public event Action<Control>? VisibleChanged;

        /// <summary>
        /// Gets or sets the diagnostic name of this control.
        /// </summary>
        public string Name { get; set; } = string.Empty;

        /// <summary>
        /// Gets or sets the text presented by the owning root tooltip when the pointer rests over this control.
        /// </summary>
        public string TooltipText { get; set; } = string.Empty;

        /// <summary>
        /// Gets the parent container, or <c>null</c> when unparented.
        /// </summary>
        public ContainerControl? Parent
        {
            get => m_Parent;
            set
            {
                if (ReferenceEquals(m_Parent, value))
                    return;

                value?.AddChild(this);
                if (value == null)
                    m_Parent?.RemoveChild(this);
            }
        }

        /// <summary>
        /// Checks if control has parent container control.
        /// </summary>
        public bool HasParent => m_Parent != null;

        /// <summary>
        /// Gets the root control that owns this control tree.
        /// </summary>
        public RootControl? Root => m_Root;

        /// <summary>
        /// Gets or sets the zero-based position of this control inside its parent.
        /// </summary>
        public int IndexInParent
        {
            get => m_Parent?.IndexOf(this) ?? -1;
            set => m_Parent?.SetChildIndex(this, value);
        }

        /// <summary>
        /// Gets or sets the local bounds relative to the parent control.
        /// </summary>
        public Rectangle Bounds
        {
            get => m_Bounds;
            set => SetBounds(value);
        }

        public float X
        {
            get => m_Bounds.X;
            set => SetBounds(value, m_Bounds.Y, m_Bounds.Width, m_Bounds.Height);
        }

        public float Y
        {
            get => m_Bounds.Y;
            set => SetBounds(m_Bounds.X, value, m_Bounds.Width, m_Bounds.Height);
        }

        public float Width
        {
            get => m_Bounds.Width;
            set => SetBounds(m_Bounds.X, m_Bounds.Y, value, m_Bounds.Height);
        }

        public float Height
        {
            get => m_Bounds.Height;
            set => SetBounds(m_Bounds.X, m_Bounds.Y, m_Bounds.Width, value);
        }

        public Float2 Location
        {
            get => m_Bounds.Location;
            set => SetBounds(new Rectangle(value, m_Bounds.Size));
        }

        public Float2 Size
        {
            get => m_Bounds.Size;
            set => SetBounds(new Rectangle(m_Bounds.Location, value));
        }

        /// <summary>
        /// Gets the bounds in root logical coordinates.
        /// </summary>
        public Rectangle ScreenBounds => new Rectangle(ScreenPos, m_Bounds.Size);

        /// <summary>
        /// Gets the position in root logical coordinates.
        /// </summary>
        public Float2 ScreenPos
        {
            get
            {
                var position = m_Bounds.Location;
                for (Control child = this; child.Parent is ContainerControl parent; child = parent)
                {
                    position += parent.Location;
                    if (child.ApplyParentChildOffset)
                        position += parent.ChildOffset;
                }
                return position;
            }
        }

        /// <summary>
        /// Gets or sets the normalized minimum anchor point.
        /// </summary>
        public Float2 AnchorMin { get; set; } = Float2.Zero;

        /// <summary>
        /// Gets or sets the normalized maximum anchor point.
        /// </summary>
        public Float2 AnchorMax { get; set; } = Float2.Zero;

        /// <summary>
        /// Gets or sets the offsets relative to the anchors.
        /// </summary>
        public Margin Offsets { get; set; } = new Margin(0.0f, 100.0f, 0.0f, 30.0f);

        /// <summary>
        /// Gets or sets the normalized pivot used by layout implementations.
        /// </summary>
        public Float2 Pivot { get; set; } = Float2.Half;

        /// <summary>
        /// Gets or sets the visual scale applied by a control implementation.
        /// </summary>
        public Float2 Scale { get; set; } = Float2.One;

        /// <summary>
        /// Gets or sets the visual rotation in degrees.
        /// </summary>
        public float Rotation { get; set; }

        /// <summary>
        /// Gets or sets whether the control can receive user input.
        /// </summary>
        public bool Enabled
        {
            get => m_Enabled;
            set
            {
                if (m_Enabled == value)
                    return;

                m_Enabled = value;
                if (!value)
                    ClearState();
            }
        }

        /// <summary>
        /// Gets whether this control and all its parents are enabled.
        /// </summary>
        public bool EnabledInHierarchy => m_Enabled && (m_Parent?.EnabledInHierarchy ?? true);

        /// <summary>
        /// Gets or sets whether the control participates in layout, drawing and hit testing.
        /// </summary>
        public bool Visible
        {
            get => m_Visible;
            set
            {
                if (m_Visible == value)
                    return;

                m_Visible = value;
                if (!value)
                    ClearState();
                VisibleChanged?.Invoke(this);
            }
        }

        /// <summary>
        /// Gets whether this control and all its parents are visible.
        /// </summary>
        public bool VisibleInHierarchy => m_Visible && (m_Parent?.VisibleInHierarchy ?? true);

        /// <summary>
        /// Gets or sets whether pointer selection automatically focuses this control.
        /// </summary>
        public bool AutoFocus { get; set; } = true;

        /// <summary>
        /// Gets or sets whether scroll containers should include this control.
        /// </summary>
        public bool IsScrollable { get; set; } = true;

        /// <summary>
        /// Gets whether this control is moved by the view offset of a scrollable parent.
        /// Internal overlay controls, such as scrollbars, opt out of the offset.
        /// </summary>
        internal virtual bool ApplyParentChildOffset => true;

        /// <summary>
        /// Gets or sets the background color used by controls that render a background.
        /// </summary>
        public Color BackgroundColor { get; set; }

        public bool IsMouseOver => m_IsMouseOver;
        public bool IsFocused => m_IsFocused;
        public virtual bool ContainsFocus => m_IsFocused;
        public bool IsDisposing => m_IsDisposing;
        public bool IsDisposed => m_IsDisposed;

        /// <summary>
        /// Sets the local bounds.
        /// </summary>
        public void SetBounds(float x, float y, float width, float height)
        {
            SetBounds(new Rectangle(x, y, width, height));
        }

        /// <summary>
        /// Sets the local bounds.
        /// </summary>
        public void SetBounds(Rectangle bounds)
        {
            if (m_Bounds.Equals(bounds))
                return;

            bool locationChanged = m_Bounds.Location != bounds.Location;
            bool sizeChanged = m_Bounds.Size != bounds.Size;
            m_Bounds = bounds;

            if (locationChanged)
                LocationChanged?.Invoke(this);
            if (sizeChanged)
                SizeChanged?.Invoke(this);

            OnBoundsChanged(locationChanged, sizeChanged);
        }

        /// <summary>
        /// Converts a point from root logical coordinates to local control coordinates.
        /// </summary>
        public Float2 PointFromRoot(Float2 location)
        {
            return location - ScreenPos;
        }

        /// <summary>
        /// Converts a point from local control coordinates to root logical coordinates.
        /// </summary>
        public Float2 PointToRoot(Float2 location)
        {
            return location + ScreenPos;
        }

        /// <summary>
        /// Converts a point from local control coordinates to immediate parent coordinates.
        /// </summary>
        public Float2 PointToParent(Float2 location)
        {
            return PointToParent(ref location);
        }

        /// <summary>
        /// Converts a point from local control coordinates to immediate parent coordinates.
        /// </summary>
        public virtual Float2 PointToParent(ref Float2 location)
        {
            Float2 result = location + Location;
            if (ApplyParentChildOffset && Parent != null)
                result += Parent.ChildOffset;
            return result;
        }

        /// <summary>
        /// Converts a point from local control coordinates to one of ancestor parent coordinates.
        /// </summary>
        public Float2 PointToParent(ContainerControl parent, Float2 location)
        {
            ArgumentNullException.ThrowIfNull(parent);

            Control? control = this;
            while (control != null && !ReferenceEquals(control, parent))
            {
                location = control.PointToParent(location);
                control = control.Parent;
            }

            return location;
        }

        /// <summary>
        /// Converts a point from immediate parent coordinates to local control coordinates.
        /// </summary>
        public Float2 PointFromParent(Float2 locationParent)
        {
            return PointFromParent(ref locationParent);
        }

        /// <summary>
        /// Converts a point from immediate parent coordinates to local control coordinates.
        /// </summary>
        public virtual Float2 PointFromParent(ref Float2 locationParent)
        {
            Float2 result = locationParent;
            if (ApplyParentChildOffset && Parent != null)
                result -= Parent.ChildOffset;
            result -= Location;
            return result;
        }

        /// <summary>
        /// Converts a point from one of ancestor parent coordinates to local control coordinates.
        /// </summary>
        public Float2 PointFromParent(ContainerControl parent, Float2 location)
        {
            ArgumentNullException.ThrowIfNull(parent);

            List<Control> path = new List<Control>();
            Control? control = this;
            while (control != null && !ReferenceEquals(control, parent))
            {
                path.Add(control);
                control = control.Parent;
            }

            for (int i = path.Count - 1; i >= 0; i--)
                location = path[i].PointFromParent(location);

            return location;
        }

        /// <summary>
        /// Checks if a root logical-coordinate point is inside this control.
        /// </summary>
        public virtual bool ContainsPoint(Float2 location)
        {
            return VisibleInHierarchy && ScreenBounds.Contains(location);
        }

        /// <summary>
        /// Performs layout for this control.
        /// </summary>
        public virtual void PerformLayout(bool force = false)
        {
            _ = force;
        }

        /// <summary>
        /// Updates this control once per frame.
        /// </summary>
        public virtual void Update(float deltaTime)
        {
            if (VisibleInHierarchy && !m_IsDisposed)
                OnUpdate(deltaTime);
        }

        /// <summary>
        /// Draws this control. Drawing is intentionally performed in logical coordinates.
        /// </summary>
        public virtual void Draw()
        {
            if (VisibleInHierarchy && !m_IsDisposed)
            {
                OnDraw();
            }
        }


        /// <summary>
        /// When control gets input focus
        /// </summary>
        public virtual void OnGetFocus()
        {
            // Cache flag
            m_IsFocused = true;
            // _isNavFocused = false;
        }

        /// <summary>
        /// When control losts input focus
        /// </summary>
        public virtual void OnLostFocus()
        {
            // Clear flag
            m_IsFocused = false;
            // _isNavFocused = false;
        }

        /// <summary>
        /// Sets input focus to the control.
        /// </summary>
        public virtual void Focus()
        {
            if (!IsFocused)
                Root?.Focus(this);
        }

        /// <summary>
        /// Removes input focus from the control.
        /// </summary>
        public virtual void Defocus()
        {
            if (ContainsFocus)
                Root?.Focus(null);
        }

        /// <summary>
        /// Called when control starts containing focus.
        /// </summary>
        public virtual void OnStartContainsFocus()
        {
        }

        /// <summary>
        /// Called when control stops containing focus.
        /// </summary>
        public virtual void OnEndContainsFocus()
        {
        }

        /// <summary>
        /// Clears transient input state from this control.
        /// </summary>
        public virtual void ClearState()
        {
            SetMouseOver(false);
            Defocus();

            if (m_IsMouseOver)
                OnMouseLeave();
            if (m_IsDragOver)
                OnDragLeave();
            /*while (_touchOvers != null && _touchOvers.Count != 0)
            {
                OnTouchLeave(_touchOvers[0]);
            }*/
        }

        /// <summary>
        /// Disposes the control and detaches it from its parent.
        /// </summary>
        public void Dispose()
        {
            if (m_IsDisposed || m_IsDisposing)
                return;

            m_IsDisposing = true;
            m_Parent?.RemoveChild(this);
            ClearState();
            OnDispose();
            m_IsDisposed = true;
            m_IsDisposing = false;
            GC.SuppressFinalize(this);
        }

        /// <summary>
        /// Handles character input routed from the root control.
        /// </summary>
        public virtual bool OnCharInput(char character)
        {
            _ = character;
            return false;
        }

        /// <summary>
        /// Handles a key press routed from the root control.
        /// </summary>
        public virtual bool OnKeyDown(KeyboardKeys key)
        {
            _ = key;
            return false;
        }

        /// <summary>
        /// Handles a key release routed from the root control.
        /// </summary>
        public virtual bool OnKeyUp(KeyboardKeys key)
        {
            _ = key;
            return false;
        }

        public virtual bool OnMouseDown(Float2 location, MouseButton button)
        {
            _ = location;
            _ = button;
            return false;
        }

        public virtual bool OnMouseUp(Float2 location, MouseButton button)
        {
            _ = location;
            _ = button;
            return false;
        }

        public virtual bool OnMouseDoubleClick(Float2 location, MouseButton button)
        {
            _ = location;
            _ = button;
            return false;
        }

        public virtual bool OnMouseWheel(Float2 location, float delta)
        {
            _ = location;
            _ = delta;
            return false;
        }

        public virtual void OnMouseMove(Float2 location)
        {
            _ = location;
        }

        public virtual void OnMouseEnter()
        {
        }

        public virtual void OnMouseLeave()
        {
        }

        public virtual bool OnTouchDown(Float2 location, int pointerIndex)
        {
            _ = location;
            _ = pointerIndex;
            return false;
        }

        public virtual void OnTouchMove(Float2 location, int pointerIndex)
        {
            _ = location;
            _ = pointerIndex;
        }

        public virtual bool OnTouchUp(Float2 location, int pointerIndex)
        {
            _ = location;
            _ = pointerIndex;
            return false;
        }

        public virtual void OnFocusGained()
        {
        }

        public virtual void OnFocusLost()
        {
        }

        public virtual DragDropEffect OnDragEnter(ref Float2 location, DragData data)
        {
            _ = location;
            _ = data;
            return DragDropEffect.None;
        }

        public virtual DragDropEffect OnDragMove(ref Float2 location, DragData data)
        {
            _ = location;
            _ = data;
            return DragDropEffect.None;
        }

        public virtual DragDropEffect OnDragDrop(ref Float2 location, DragData data)
        {
            _ = location;
            _ = data;
            return DragDropEffect.None;
        }

        public virtual void OnDragLeave()
        {
        }

        protected virtual void OnUpdate(float deltaTime)
        {
            _ = deltaTime;
        }

        protected virtual void OnDraw()
        {
            if (BackgroundColor.A <= 0.0f)
                return;

            Rectangle bounds = ScreenBounds;
            Color color = BackgroundColor;
            Render2D.FillRectangle(ref bounds, ref color);
        }

        protected virtual void OnDispose()
        {
        }

        protected virtual void OnBoundsChanged(bool locationChanged, bool sizeChanged)
        {
            _ = locationChanged;
            if (sizeChanged)
                PerformLayout();
        }

        internal void SetParentCore(ContainerControl? parent)
        {
            if (ReferenceEquals(m_Parent, parent))
                return;

            ContainerControl? oldParent = m_Parent;
            m_Parent = parent;
            SetRootCore(parent?.Root);
            ParentChanged?.Invoke(this);
            oldParent?.UpdateContainsFocusUpwards();
            parent?.UpdateContainsFocusUpwards();
        }

        internal virtual void SetRootCore(RootControl? root)
        {
            if (ReferenceEquals(m_Root, root))
                return;

            if (m_IsFocused && !ReferenceEquals(root, m_Root))
                m_Root?.Focus(null);
            m_Root = root;
        }

        internal void SetMouseOver(bool value)
        {
            if (m_IsMouseOver == value)
                return;

            m_IsMouseOver = value;
            if (value)
                OnMouseEnter();
            else
                OnMouseLeave();
        }

        internal void SetFocused(bool value)
        {
            if (m_IsFocused == value)
                return;

            m_IsFocused = value;
            if (value)
            {
                OnGetFocus();
                OnFocusGained();
            }
            else
            {
                OnLostFocus();
                OnFocusLost();
            }
            m_IsFocused = value;
            if (this is ContainerControl container)
                container.UpdateContainsFocusUpwards();
            else
                m_Parent?.UpdateContainsFocusUpwards();
        }
    }
}
