using System;

namespace SE.GUI
{
    /// <summary>
    /// Base class for all managed GUI controls.
    /// </summary>
    public class Control : IDisposable
    {
        private ContainerControl? _parent;
        private RootControl? _root;
        private Rectangle _bounds;
        private bool _visible = true;
        private bool _enabled = true;
        private bool _isDisposing;
        private bool _isDisposed;
        private bool _isMouseOver;
        private bool _isFocused;

        /// <summary>
        /// Initializes a new instance of the <see cref="Control"/> class.
        /// </summary>
        public Control() : this(Rectangle.Empty)
        {
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="Control"/> class.
        /// </summary>
        public Control(Rectangle bounds)
        {
            _bounds = bounds;
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
            get => _parent;
            set
            {
                if (ReferenceEquals(_parent, value))
                    return;

                value?.AddChild(this);
                if (value == null)
                    _parent?.RemoveChild(this);
            }
        }

        /// <summary>
        /// Gets the root control that owns this control tree.
        /// </summary>
        public RootControl? Root => _root;

        /// <summary>
        /// Gets or sets the zero-based position of this control inside its parent.
        /// </summary>
        public int IndexInParent
        {
            get => _parent?.IndexOf(this) ?? -1;
            set => _parent?.SetChildIndex(this, value);
        }

        /// <summary>
        /// Gets or sets the local bounds relative to the parent control.
        /// </summary>
        public Rectangle Bounds
        {
            get => _bounds;
            set => SetBounds(value);
        }

        public float X
        {
            get => _bounds.X;
            set => SetBounds(value, _bounds.Y, _bounds.Width, _bounds.Height);
        }

        public float Y
        {
            get => _bounds.Y;
            set => SetBounds(_bounds.X, value, _bounds.Width, _bounds.Height);
        }

        public float Width
        {
            get => _bounds.Width;
            set => SetBounds(_bounds.X, _bounds.Y, value, _bounds.Height);
        }

        public float Height
        {
            get => _bounds.Height;
            set => SetBounds(_bounds.X, _bounds.Y, _bounds.Width, value);
        }

        public Float2 Location
        {
            get => _bounds.Location;
            set => SetBounds(new Rectangle(value, _bounds.Size));
        }

        public Float2 Size
        {
            get => _bounds.Size;
            set => SetBounds(new Rectangle(_bounds.Location, value));
        }

        /// <summary>
        /// Gets the bounds in root logical coordinates.
        /// </summary>
        public Rectangle ScreenBounds => new Rectangle(ScreenPos, _bounds.Size);

        /// <summary>
        /// Gets the position in root logical coordinates.
        /// </summary>
        public Float2 ScreenPos
        {
            get
            {
                var position = _bounds.Location;
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
            get => _enabled;
            set
            {
                if (_enabled == value)
                    return;

                _enabled = value;
                if (!value)
                    ClearState();
            }
        }

        /// <summary>
        /// Gets whether this control and all its parents are enabled.
        /// </summary>
        public bool EnabledInHierarchy => _enabled && (_parent?.EnabledInHierarchy ?? true);

        /// <summary>
        /// Gets or sets whether the control participates in layout, drawing and hit testing.
        /// </summary>
        public bool Visible
        {
            get => _visible;
            set
            {
                if (_visible == value)
                    return;

                _visible = value;
                if (!value)
                    ClearState();
                VisibleChanged?.Invoke(this);
            }
        }

        /// <summary>
        /// Gets whether this control and all its parents are visible.
        /// </summary>
        public bool VisibleInHierarchy => _visible && (_parent?.VisibleInHierarchy ?? true);

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

        public bool IsMouseOver => _isMouseOver;
        public bool IsFocused => _isFocused;
        public bool IsDisposing => _isDisposing;
        public bool IsDisposed => _isDisposed;

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
            if (_bounds.Equals(bounds))
                return;

            bool locationChanged = _bounds.Location != bounds.Location;
            bool sizeChanged = _bounds.Size != bounds.Size;
            _bounds = bounds;

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
            if (VisibleInHierarchy && !_isDisposed)
                OnUpdate(deltaTime);
        }

        /// <summary>
        /// Draws this control. Drawing is intentionally performed in logical coordinates.
        /// </summary>
        public virtual void Draw()
        {
            if (VisibleInHierarchy && !_isDisposed)
                OnDraw();
        }

        /// <summary>
        /// Clears transient input state from this control.
        /// </summary>
        public virtual void ClearState()
        {
            SetMouseOver(false);
            if (_isFocused)
                _root?.Focus(null);
        }

        /// <summary>
        /// Disposes the control and detaches it from its parent.
        /// </summary>
        public void Dispose()
        {
            if (_isDisposed || _isDisposing)
                return;

            _isDisposing = true;
            _parent?.RemoveChild(this);
            ClearState();
            OnDispose();
            _isDisposed = true;
            _isDisposing = false;
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
        public virtual bool OnKeyDown(int key)
        {
            _ = key;
            return false;
        }

        /// <summary>
        /// Handles a key release routed from the root control.
        /// </summary>
        public virtual bool OnKeyUp(int key)
        {
            _ = key;
            return false;
        }

        public virtual bool OnMouseDown(Float2 location, int button)
        {
            _ = location;
            _ = button;
            return false;
        }

        public virtual bool OnMouseUp(Float2 location, int button)
        {
            _ = location;
            _ = button;
            return false;
        }

        public virtual bool OnMouseDoubleClick(Float2 location, int button)
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

        public virtual DragDropEffect OnDragEnter(Float2 location, DragData data)
        {
            _ = location;
            _ = data;
            return DragDropEffect.None;
        }

        public virtual DragDropEffect OnDragMove(Float2 location, DragData data)
        {
            _ = location;
            _ = data;
            return DragDropEffect.None;
        }

        public virtual DragDropEffect OnDragDrop(Float2 location, DragData data)
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
            if (ReferenceEquals(_parent, parent))
                return;

            _parent = parent;
            SetRootCore(parent?.Root);
            ParentChanged?.Invoke(this);
        }

        internal virtual void SetRootCore(RootControl? root)
        {
            if (ReferenceEquals(_root, root))
                return;

            if (_isFocused && !ReferenceEquals(root, _root))
                _root?.Focus(null);
            _root = root;
        }

        internal void SetMouseOver(bool value)
        {
            if (_isMouseOver == value)
                return;

            _isMouseOver = value;
            if (value)
                OnMouseEnter();
            else
                OnMouseLeave();
        }

        internal void SetFocused(bool value)
        {
            if (_isFocused == value)
                return;

            _isFocused = value;
            if (value)
                OnFocusGained();
            else
                OnFocusLost();
        }
    }
}
