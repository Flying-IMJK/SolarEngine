using System;
using System.Collections.Generic;

namespace SE.GUI
{
    /// <summary>Base class for all GUI controls.</summary>
    public class Control : IDisposable
    {
        public struct AnchorPresetData
        {
            public AnchorPresets Preset;
            public Float2 Min;
            public Float2 Max;

            public AnchorPresetData(AnchorPresets preset, Float2 min, Float2 max)
            {
                Preset = preset;
                Min = min;
                Max = max;
            }
        }

        protected static readonly AnchorPresetData[] anchorPresetsData =
        {
            new(AnchorPresets.TopLeft, Float2.Zero, Float2.Zero),
            new(AnchorPresets.TopCenter, new Float2(0.5f, 0), new Float2(0.5f, 0)),
            new(AnchorPresets.TopRight, new Float2(1, 0), new Float2(1, 0)),
            new(AnchorPresets.MiddleLeft, new Float2(0, 0.5f), new Float2(0, 0.5f)),
            new(AnchorPresets.MiddleCenter, new Float2(0.5f), new Float2(0.5f)),
            new(AnchorPresets.MiddleRight, new Float2(1, 0.5f), new Float2(1, 0.5f)),
            new(AnchorPresets.BottomLeft, new Float2(0, 1), new Float2(0, 1)),
            new(AnchorPresets.BottomCenter, new Float2(0.5f, 1), new Float2(0.5f, 1)),
            new(AnchorPresets.BottomRight, Float2.One, Float2.One),
            new(AnchorPresets.HorizontalStretchTop, Float2.Zero, new Float2(1, 0)),
            new(AnchorPresets.HorizontalStretchMiddle, new Float2(0, 0.5f), new Float2(1, 0.5f)),
            new(AnchorPresets.HorizontalStretchBottom, new Float2(0, 1), Float2.One),
            new(AnchorPresets.VerticalStretchLeft, Float2.Zero, new Float2(0, 1)),
            new(AnchorPresets.VerticalStretchCenter, new Float2(0.5f, 0), new Float2(0.5f, 1)),
            new(AnchorPresets.VerticalStretchRight, new Float2(1, 0), Float2.One),
            new(AnchorPresets.StretchAll, Float2.Zero, Float2.One),
        };

        private ContainerControl? m_Parent;
        private RootControl? m_Root;
        private bool m_Visible = true;
        private bool m_Enabled = true;
        private bool m_IsDisposing;
        private bool m_IsDisposed;
        private bool m_IsMouseOver;
        private bool m_IsDragOver;
        private bool m_IsFocused;
        private bool m_IsNavFocused;
        private readonly List<int> m_TouchOvers = new();

        private Rectangle m_Bounds;
        private Margin m_Offsets;
        private Float2 m_AnchorMin;
        private Float2 m_AnchorMax;
        private Float2 m_Scale = Float2.One;
        private Float2 m_Pivot = Float2.Half;
        private Float2 m_Shear;
        private float m_Rotation;
        private Matrix3x3 m_CachedTransform;
        private Matrix3x3 m_CachedTransformInv;
        private bool m_PivotRelativeSizing;

        public Control() : this(Rectangle.Empty) { }
        public Control(float x, float y, float width, float height) : this(new Rectangle(x, y, width, height)) { }
        public Control(Float2 location, Float2 size) : this(new Rectangle(location, size)) { }

        public Control(Rectangle bounds)
        {
            m_Bounds = bounds;
            m_Offsets = new Margin(bounds.X, bounds.Width, bounds.Y, bounds.Height);
            UpdateTransform();
        }

        public event Action<Control>? LocationChanged;
        public event Action<Control>? SizeChanged;
        public event Action<Control>? ParentChanged;
        public event Action<Control>? VisibleChanged;

        public string Name { get; set; } = string.Empty;
        public string TooltipText { get; set; } = string.Empty;

        public ContainerControl? Parent
        {
            get => m_Parent;
            set
            {
                if (ReferenceEquals(m_Parent, value))
                    return;

                Defocus();
                Float2 oldParentSize;
                if (m_Parent != null)
                {
                    oldParentSize = m_Parent.Size;
                    m_Parent.RemoveChildInternal(this);
                }
                else
                {
                    oldParentSize = Float2.Zero;
                    ClearState();
                }

                m_Parent = value;
                if (m_Parent != null)
                    m_Parent.AddChildInternal(this);

                CacheRootHandle();
                OnParentChangedInternal();
                if (m_Parent != null && oldParentSize != m_Parent.Size)
                    OnParentResized();
            }
        }

        public bool HasParent => m_Parent != null;
        public RootControl? Root => m_Root;
        public int IndexInParent { get => m_Parent?.IndexOf(this) ?? -1; set => m_Parent?.SetChildIndex(this, value); }

        public AnchorPresets AnchorPreset
        {
            get
            {
                for (int i = 0; i < anchorPresetsData.Length; i++)
                {
                    if (Float2.NearEqual(m_AnchorMin, anchorPresetsData[i].Min) && Float2.NearEqual(m_AnchorMax, anchorPresetsData[i].Max))
                        return anchorPresetsData[i].Preset;
                }
                return AnchorPresets.Custom;
            }
            set => SetAnchorPreset(value, false);
        }

        public Rectangle Bounds { get => m_Bounds; set => SetBounds(value); }
        public float X { get => m_Bounds.X; set => SetBounds(value, Y, Width, Height); }
        public float Y { get => m_Bounds.Y; set => SetBounds(X, value, Width, Height); }
        public float Width
        {
            get => m_Bounds.Width;
            set
            {
                if (Mathf.NearEqual(Width, value)) return;
                Rectangle bounds = new(X, Y, value, Height);
                if (m_PivotRelativeSizing) bounds.X += (Width - value) * m_Pivot.X;
                SetBounds(bounds);
            }
        }
        public float Height
        {
            get => m_Bounds.Height;
            set
            {
                if (Mathf.NearEqual(Height, value)) return;
                Rectangle bounds = new(X, Y, Width, value);
                if (m_PivotRelativeSizing) bounds.Y += (Height - value) * m_Pivot.Y;
                SetBounds(bounds);
            }
        }
        public Float2 Location { get => m_Bounds.Location; set => SetBounds(new Rectangle(value, Size)); }
        public Float2 Size { get => m_Bounds.Size; set => SetBounds(new Rectangle(Location, value)); }
        public Float2 Center { get => m_Bounds.Center; set => Location = value - Size * 0.5f; }
        public Float2 UpperLeft => m_Bounds.UpperLeft;
        public Float2 UpperRight => m_Bounds.UpperRight;
        public Float2 BottomRight => m_Bounds.BottomRight;
        public Float2 BottomLeft => m_Bounds.BottomLeft;
        public Rectangle ScreenBounds
        {
            get
            {
                Float2 upperLeft = PointToRoot(Float2.Zero);
                Float2 bottomRight = PointToRoot(Size);
                return new Rectangle(upperLeft, bottomRight - upperLeft);
            }
        }
        public Float2 ScreenPos => PointToRoot(Float2.Zero);

        public Float2 LocalLocation
        {
            get
            {
                Float2 localLocation = Location;
                if (m_Parent != null)
                    _ = localLocation - (m_Parent.m_Bounds.Size * (m_AnchorMax + m_AnchorMin) * 0.5f);
                localLocation += Size * m_Pivot;
                return localLocation;
            }
            set
            {
                if (m_Parent != null)
                    SetBounds(new Rectangle(value + (m_Parent.Bounds.Size * (m_AnchorMax + m_AnchorMin) * 0.5f) - Size * m_Pivot, Size));
                else
                    SetBounds(new Rectangle(value - Size * m_Pivot, Size));
            }
        }
        public float LocalX { get => LocalLocation.X; set => LocalLocation = new Float2(value, LocalLocation.Y); }
        public float LocalY { get => LocalLocation.Y; set => LocalLocation = new Float2(LocalLocation.X, value); }
        public bool PivotRelativeSizing { get => m_PivotRelativeSizing; set => m_PivotRelativeSizing = value; }

        public Float2 AnchorMin
        {
            get => m_AnchorMin;
            set { if (m_AnchorMin != value) { Rectangle bounds = m_Bounds; m_AnchorMin = value; UpdateBounds(); SetBounds(bounds); } }
        }
        public Float2 AnchorMax
        {
            get => m_AnchorMax;
            set { if (m_AnchorMax != value) { Rectangle bounds = m_Bounds; m_AnchorMax = value; UpdateBounds(); SetBounds(bounds); } }
        }
        public Margin Offsets { get => m_Offsets; set { if (m_Offsets != value) { m_Offsets = value; UpdateBounds(); } } }
        public Float2 Pivot { get => m_Pivot; set { if (m_Pivot != value) { m_Pivot = value; UpdateTransform(); m_Parent?.OnChildResized(this); } } }
        public Float2 Scale { get => m_Scale; set { if (m_Scale != value) { m_Scale = value; UpdateTransform(); m_Parent?.OnChildResized(this); } } }
        public Float2 Shear { get => m_Shear; set { if (m_Shear != value) { m_Shear = value; UpdateTransform(); m_Parent?.OnChildResized(this); } } }
        public float Rotation { get => m_Rotation; set { if (!Mathf.NearEqual(m_Rotation, value)) { m_Rotation = value; UpdateTransform(); m_Parent?.OnChildResized(this); } } }
        public Matrix3x3 CachedTransform => m_CachedTransform;

        public bool Enabled { get => m_Enabled; set { if (m_Enabled != value) { m_Enabled = value; if (!value) ClearState(); } } }
        public bool EnabledInHierarchy => m_Enabled && (m_Parent?.EnabledInHierarchy ?? true);
        public bool Visible
        {
            get => m_Visible;
            set
            {
                if (m_Visible == value) return;
                m_Visible = value;
                if (!value) ClearState();
                VisibleChanged?.Invoke(this);
                m_Parent?.PerformLayout();
            }
        }
        public bool VisibleInHierarchy => m_Visible && (m_Parent?.VisibleInHierarchy ?? true);
        public bool AutoFocus { get; set; } = true;
        public bool IsScrollable { get; set; } = true;
        public Color BackgroundColor { get; set; } = Color.Transparent;
        public bool IsMouseOver => m_IsMouseOver;
        public bool IsFocused => m_IsFocused;
        public bool IsNavFocused => m_IsNavFocused;
        public virtual bool ContainsFocus => m_IsFocused;
        public bool IsDisposing => m_IsDisposing;
        public bool IsDisposed => m_IsDisposed;
        internal virtual bool ApplyParentChildOffset => true;

        public Control? NavTargetUp { get; set; }
        public Control? NavTargetDown { get; set; }
        public Control? NavTargetLeft { get; set; }
        public Control? NavTargetRight { get; set; }

        public void SetAnchorPreset(AnchorPresets anchorPreset, bool preserveBounds, bool setPivotToo = false)
        {
            for (int i = 0; i < anchorPresetsData.Length; i++)
            {
                if (anchorPresetsData[i].Preset != anchorPreset) continue;
                Float2 anchorMin = anchorPresetsData[i].Min;
                Float2 anchorMax = anchorPresetsData[i].Max;
                Rectangle bounds = m_Bounds;
                if (!Float2.NearEqual(m_AnchorMin, anchorMin) || !Float2.NearEqual(m_AnchorMax, anchorMax))
                {
                    if (!anchorMin.IsZero || !anchorMax.IsZero) IsScrollable = false;
                    m_AnchorMin = anchorMin;
                    m_AnchorMax = anchorMax;
                    if (preserveBounds) { UpdateBounds(); SetBounds(bounds); }
                }
                if (!preserveBounds)
                {
                    if (m_Parent != null)
                    {
                        Rectangle parentBounds = m_Parent.GetDesireClientArea();
                        switch (anchorPreset)
                        {
                            case AnchorPresets.TopLeft: bounds.Location = Float2.Zero; break;
                            case AnchorPresets.TopCenter: bounds.Location = new Float2(parentBounds.Left * 0.5f - bounds.Left * 0.5f, 0); break;
                            case AnchorPresets.TopRight: bounds.Location = new Float2(parentBounds.Left - bounds.Left, 0); break;
                            case AnchorPresets.MiddleLeft: bounds.Location = new Float2(0, parentBounds.Height * 0.5f - bounds.Height * 0.5f); break;
                            case AnchorPresets.MiddleCenter: bounds.Location = new Float2(parentBounds.Left * 0.5f - bounds.Left * 0.5f, parentBounds.Height * 0.5f - bounds.Height * 0.5f); break;
                            case AnchorPresets.MiddleRight: bounds.Location = new Float2(parentBounds.Left - bounds.Left, parentBounds.Height * 0.5f - bounds.Height * 0.5f); break;
                            case AnchorPresets.BottomLeft: bounds.Location = new Float2(0, parentBounds.Height - bounds.Height); break;
                            case AnchorPresets.BottomCenter: bounds.Location = new Float2(parentBounds.Left * 0.5f - bounds.Left * 0.5f, parentBounds.Height - bounds.Height); break;
                            case AnchorPresets.BottomRight: bounds.Location = new Float2(parentBounds.Left - bounds.Left, parentBounds.Height - bounds.Height); break;
                            case AnchorPresets.VerticalStretchLeft: bounds.Location = Float2.Zero; bounds.Size = new Float2(bounds.Left, parentBounds.Height); break;
                            case AnchorPresets.VerticalStretchCenter: bounds.Location = new Float2(parentBounds.Left * 0.5f - bounds.Left * 0.5f, 0); bounds.Size = new Float2(bounds.Left, parentBounds.Height); break;
                            case AnchorPresets.VerticalStretchRight: bounds.Location = new Float2(parentBounds.Left - bounds.Left, 0); bounds.Size = new Float2(bounds.Left, parentBounds.Height); break;
                            case AnchorPresets.HorizontalStretchTop: bounds.Location = Float2.Zero; bounds.Size = new Float2(parentBounds.Left, bounds.Height); break;
                            case AnchorPresets.HorizontalStretchMiddle: bounds.Location = new Float2(0, parentBounds.Height * 0.5f - bounds.Height * 0.5f); bounds.Size = new Float2(parentBounds.Left, bounds.Height); break;
                            case AnchorPresets.HorizontalStretchBottom: bounds.Location = new Float2(0, parentBounds.Height - bounds.Height); bounds.Size = new Float2(parentBounds.Left, bounds.Height); break;
                            case AnchorPresets.StretchAll: bounds.Location = Float2.Zero; bounds.Size = parentBounds.Size; break;
                        }
                        bounds.Location += parentBounds.Location;
                    }
                    SetBounds(bounds);
                }
                if (setPivotToo) Pivot = (anchorMin + anchorMax) / 2.0f;
                m_Parent?.PerformLayout();
                return;
            }
        }

        public void SetBounds(float x, float y, float width, float height) => SetBounds(new Rectangle(x, y, width, height));
        public void SetBounds(Rectangle bounds)
        {
            if (m_Bounds == bounds) return;
            // Native bounds assignment first derives offsets from the parent client area.
            Rectangle parentBounds = m_Parent?.GetDesireClientArea() ?? Rectangle.Empty;
            Margin anchors = m_Parent == null ? Margin.Zero : new Margin(
                m_AnchorMin.X * parentBounds.Size.X + parentBounds.Location.X,
                m_AnchorMax.X * parentBounds.Size.X,
                m_AnchorMin.Y * parentBounds.Size.Y + parentBounds.Location.Y,
                m_AnchorMax.Y * parentBounds.Size.Y);
            m_Offsets.Left = bounds.X - anchors.Left;
            m_Offsets.Right = m_AnchorMin.X != m_AnchorMax.X ? anchors.Right - bounds.X - bounds.Width : bounds.Width;
            m_Offsets.Top = bounds.Y - anchors.Top;
            m_Offsets.Bottom = m_AnchorMin.Y != m_AnchorMax.Y ? anchors.Bottom - bounds.Y - bounds.Height : bounds.Height;
            UpdateBounds();
        }

        internal void UpdateBounds()
        {
            Rectangle previous = m_Bounds;
            Rectangle parentBounds = m_Parent?.GetDesireClientArea() ?? Rectangle.Empty;
            Float2 anchorMin = m_Parent == null ? Float2.Zero : m_AnchorMin * parentBounds.Size;
            Float2 anchorMax = m_Parent == null ? Float2.Zero : m_AnchorMax * parentBounds.Size;
            Float2 offset = m_Parent == null ? Float2.Zero : parentBounds.Location;
            m_Bounds.X = anchorMin.X + m_Offsets.Left + offset.X;
            m_Bounds.Width = m_AnchorMin.X != m_AnchorMax.X ? anchorMax.X - anchorMin.X - m_Offsets.Left - m_Offsets.Right : m_Offsets.Right;
            m_Bounds.Y = anchorMin.Y + m_Offsets.Top + offset.Y;
            m_Bounds.Height = m_AnchorMin.Y != m_AnchorMax.Y ? anchorMax.Y - anchorMin.Y - m_Offsets.Top - m_Offsets.Bottom : m_Offsets.Bottom;
            UpdateTransform();
            bool locationChanged = previous.Location != m_Bounds.Location;
            bool sizeChanged = previous.Size != m_Bounds.Size;
            if (locationChanged) LocationChanged?.Invoke(this);
            if (sizeChanged) { SizeChanged?.Invoke(this); m_Parent?.OnChildResized(this); }
            OnBoundsChanged(locationChanged, sizeChanged);
        }

        private void UpdateTransform()
        {
            // Keep the native scale/shear/rotation order and pivot translation intact.
            Float2 pivot = m_Pivot * m_Bounds.Size;
            Float2 negativePivot = -pivot;
            float shearX = m_Shear.X == 0 ? 0 : 1.0f / Mathf.Tan(Mathf.DegreesToRadians * (90.0f - Mathf.Clamp(m_Shear.X, -89.0f, 89.0f)));
            float shearY = m_Shear.Y == 0 ? 0 : 1.0f / Mathf.Tan(Mathf.DegreesToRadians * (90.0f - Mathf.Clamp(m_Shear.Y, -89.0f, 89.0f)));
            Matrix3x3 transform = new(m_Scale.X, m_Scale.X * shearY, 0, m_Scale.Y * shearX, m_Scale.Y, 0, 0, 0, 1);
            float sin = Mathf.Sin(Mathf.DegreesToRadians * m_Rotation);
            float cos = Mathf.Cos(Mathf.DegreesToRadians * m_Rotation);
            transform.M11 = (m_Scale.X * cos) + (transform.M12 * -sin);
            transform.M12 = (m_Scale.X * sin) + (transform.M12 * cos);
            float m21 = (transform.M21 * cos) + (m_Scale.Y * -sin);
            transform.M22 = (transform.M21 * sin) + (m_Scale.Y * cos);
            transform.M21 = m21;
            transform.M31 = (negativePivot.X * transform.M11) + (negativePivot.Y * transform.M21) + pivot.X + m_Bounds.X;
            transform.M32 = (negativePivot.X * transform.M12) + (negativePivot.Y * transform.M22) + pivot.Y + m_Bounds.Y;
            m_CachedTransform = transform;
            Matrix3x3.Invert(ref m_CachedTransform, out m_CachedTransformInv);
        }

        public Float2 PointFromRoot(Float2 location)
        {
            if (m_Parent != null) location = m_Parent.PointFromRoot(location);
            return PointFromParent(location);
        }
        public Float2 PointToRoot(Float2 location)
        {
            location = PointToParent(location);
            return m_Parent == null ? location : m_Parent.PointToRoot(location);
        }
        public Float2 PointToParent(Float2 location)
        {
            Matrix3x3.Transform2D(ref location, ref m_CachedTransform, out Float2 result);
            if (ApplyParentChildOffset && m_Parent != null) result += m_Parent.ChildOffset;
            return result;
        }
        public virtual Float2 PointToParent(ref Float2 location) => PointToParent(location);
        public Float2 PointToParent(ContainerControl parent, Float2 location)
        {
            ArgumentNullException.ThrowIfNull(parent);
            Control? control = this;
            while (control != null)
            {
                location = control.PointToParent(location);
                control = control.Parent;
                if (ReferenceEquals(control, parent)) break;
            }
            return location;
        }
        public Float2 PointFromParent(Float2 location)
        {
            if (ApplyParentChildOffset && m_Parent != null) location -= m_Parent.ChildOffset;
            Matrix3x3.Transform2D(ref location, ref m_CachedTransformInv, out Float2 result);
            return result;
        }
        public virtual Float2 PointFromParent(ref Float2 location) => PointFromParent(location);
        public Float2 PointFromParent(ContainerControl parent, Float2 location)
        {
            ArgumentNullException.ThrowIfNull(parent);
            List<Control> path = new();
            Control? control = this;
            while (control != null && !ReferenceEquals(control, parent)) { path.Add(control); control = control.Parent; }
            for (int i = path.Count - 1; i >= 0; i--) location = path[i].PointFromParent(location);
            return location;
        }

        public virtual bool ContainsPoint(Float2 location) => ContainsPoint(location, false);
        public virtual bool ContainsPoint(Float2 location, bool precise) { _ = precise; return location.X >= 0 && location.Y >= 0 && location.X <= Width && location.Y <= Height; }
        public virtual bool RayCast(ref Float2 location, out Control? hit) { if (ContainsPoint(location, true)) { hit = this; return true; } hit = null; return false; }
        public virtual void PerformLayout(bool force = false) { _ = force; }
        public virtual void Update(float deltaTime) => OnUpdate(deltaTime);
        public virtual void Draw() => OnDraw();

        public virtual void OnGetFocus() { m_IsFocused = true; m_IsNavFocused = false; }
        public virtual void OnLostFocus() { m_IsFocused = false; m_IsNavFocused = false; }
        public virtual void OnFocusGained() { }
        public virtual void OnFocusLost() { }
        public virtual void Focus() { if (!IsFocused) Root?.Focus(this); }
        public virtual void Defocus() { if (ContainsFocus) Root?.Focus(null); }
        public virtual void OnStartContainsFocus() { }
        public virtual void OnEndContainsFocus() { }

        public virtual void ClearState()
        {
            // ClearState deliberately follows native ordering; do not pre-clear the flags.
            Defocus();
            if (m_IsMouseOver) OnMouseLeave();
            if (m_IsDragOver) OnDragLeave();
            while (m_TouchOvers.Count != 0) OnTouchLeave(m_TouchOvers[0]);
        }

        public void Dispose()
        {
            if (m_IsDisposing) return;
            OnDestroy();
            // The managed compatibility seam must unlink the disposed instance from its live parent list.
            if (m_Parent != null)
            {
                m_Parent.RemoveChildInternal(this);
                m_Parent = null;
            }
            OnDispose();
            m_IsDisposed = true;
            GC.SuppressFinalize(this);
        }

        public virtual void OnDestroy()
        {
            m_IsDisposing = true;
            Defocus();
            TooltipText = string.Empty;
        }

        public virtual bool OnCharInput(char character) { return false; }
        public virtual bool OnKeyDown(KeyboardKeys key) { return false; }
        public virtual bool OnKeyUp(KeyboardKeys key) { return false; }
        public virtual bool OnMouseDown(Float2 location, MouseButton button) { return false; }
        public virtual bool OnMouseUp(Float2 location, MouseButton button) { return false; }
        public virtual bool OnMouseDoubleClick(Float2 location, MouseButton button) { return false; }
        public virtual bool OnMouseWheel(Float2 location, float delta) { return false; }
        public virtual void OnMouseMove(Float2 location) { }
        public virtual void OnMouseEnter(Float2 location) { m_IsMouseOver = true; OnMouseEnter(); }
        public virtual void OnMouseEnter() { }
        public virtual void OnMouseLeave() { m_IsMouseOver = false; }
        public virtual bool IsTouchOver() => m_TouchOvers.Count != 0;
        public virtual bool IsTouchPointerOver(int pointerIndex) => m_TouchOvers.Contains(pointerIndex);
        public virtual void OnTouchEnter(Float2 location, int pointerIndex) { m_TouchOvers.Add(pointerIndex); }
        public virtual bool OnTouchDown(Float2 location, int pointerIndex) { return false; }
        public virtual void OnTouchMove(Float2 location, int pointerIndex) { }
        public virtual bool OnTouchUp(Float2 location, int pointerIndex) { return false; }
        public virtual void OnTouchLeave(int pointerIndex) { m_TouchOvers.Remove(pointerIndex); if (m_TouchOvers.Count == 0) OnTouchLeave(); }
        public virtual void OnTouchLeave() { }
        public virtual bool IsDragOver() => m_IsDragOver;
        public virtual DragDropEffect OnDragEnter(ref Float2 location, DragData data) 
        {  
            m_IsDragOver = true; 
            return DragDropEffect.None; 
        }

        public virtual DragDropEffect OnDragMove(ref Float2 location, DragData data) 
        {  
            return DragDropEffect.None; 
        }

        public virtual DragDropEffect OnDragDrop(ref Float2 location, DragData data) 
        {  
            m_IsDragOver = false;
            return DragDropEffect.None; 
        }

        public virtual void OnDragLeave() { m_IsDragOver = false; }

        public virtual Control? GetNavTarget(NavDirection direction) => direction switch
        {
            NavDirection.Up => NavTargetUp,
            NavDirection.Down => NavTargetDown,
            NavDirection.Left => NavTargetLeft,
            NavDirection.Right => NavTargetRight,
            _ => null,
        };
        public virtual Float2 GetNavOrigin(NavDirection direction) => direction switch
        {
            NavDirection.Up => new Float2(Size.X * 0.5f, 0),
            NavDirection.Down => new Float2(Size.X * 0.5f, Size.Y),
            NavDirection.Left => new Float2(0, Size.Y * 0.5f),
            // Preserve the active native Right-origin expression verbatim.
            NavDirection.Right => new Float2(Size.Y, Size.Y * 0.5f),
            NavDirection.Next => Float2.Zero,
            NavDirection.Previous => Size,
            _ => Size * 0.5f,
        };
        public virtual Control? OnNavigate(NavDirection direction, Float2 location, Control? caller, List<Control> visited)
        {
            if (caller == m_Parent && AutoFocus && Visible) 
                return this;

            return m_Parent?.OnNavigate(direction, PointToParent(GetNavOrigin(direction)), caller, visited);
        }
        public virtual void NavigationFocus() { Focus(); if (IsFocused) m_IsNavFocused = true; }

        public virtual int Compare(Control? other) => other == null ? 0 : (int)(Y - other.Y);
        protected virtual void OnUpdate(float deltaTime) { }
        protected virtual void OnDraw() 
        {
            if (BackgroundColor.A > 0.0f)
            {
                Render2D.FillRectangle(new Rectangle(Float2.Zero, Size), BackgroundColor);
            }
        }
        protected virtual void OnDispose() { }
        protected virtual void OnBoundsChanged(bool locationChanged, bool sizeChanged) { }
        protected virtual void OnParentChangedInternal() { ParentChanged?.Invoke(this); }
        protected internal virtual void OnParentResized() 
        {
            if (!m_AnchorMin.IsZero || !m_AnchorMax.IsZero)
            {
                UpdateBounds();
            }
        }
        internal virtual void CacheRootHandle() 
        { 
            if (m_Parent != null) 
                m_Root = m_Parent.Root; 
        }

        internal void SetParentCore(ContainerControl? parent) 
        { 
            m_Parent = parent; 
            CacheRootHandle(); 
            ParentChanged?.Invoke(this); 
        }
        internal virtual void SetRootCore(RootControl? root) 
        { 
            m_Root = root; 
        }
        internal void SetMouseOver(bool value) 
        {
            if (m_IsMouseOver == value)
            {
                return;
            }

            if (value)
            {
                OnMouseEnter(Float2.Zero);
            }
            else
            {
                OnMouseLeave();
            } 
        }
        internal void SetFocused(bool value)
        {
            if (m_IsFocused == value) return;
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
            m_Parent?.UpdateContainsFocusUpwards();
        }
    }
}
