#nullable disable annotations

using System;
using System.Collections.Generic;

namespace SE.GUI
{
    /// <summary>Base class for all GUI controls.</summary>
    public class Control : IDrawable, IDisposable, IComparable<Control>
    {
        protected struct AnchorPresetData
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

        protected ContainerControl m_Parent;
        protected RootControl m_Root;
        protected bool m_Visible = true;
        protected bool m_Enabled = true;
        protected bool m_IsDisposing;
        protected bool m_IsMouseOver;
        protected bool m_IsDragOver;
        protected bool m_IsFocused;
        protected bool m_IsNavFocused;
        protected readonly List<int> m_TouchOvers = new();
        protected Action<float> m_TooltipUpdate;
        protected Action<float> m_OnUpdateTooltip;
        protected Rectangle m_Bounds;
        protected Margin m_Offsets;
        protected Float2 m_AnchorMin;
        protected Float2 m_AnchorMax;
        protected Float2 m_Scale = Float2.One;
        protected Float2 m_Pivot = Float2.Half;
        protected Float2 m_Shear;
        protected float m_Rotation;
        protected Matrix3x3 m_CachedTransform;
        protected Matrix3x3 m_CachedTransformInv;
        protected bool m_PivotRelativeSizing;

        public string TooltipText
        {
            get;
            set;
        } = string.Empty;

        public Tooltip Tooltip;

        public bool AutoFocus
        {
            get;
            set;
        } = true;

        public Color BackgroundColor
        {
            get;
            set;
        } = Color.Transparent;

        public bool IsScrollable
        {
            get;
            set;
        } = true;

        public event Action<Control> LocationChanged;

        public event Action<Control> SizeChanged;

        public event Action<Control> ParentChanged;

        public event Action<Control> VisibleChanged;

        #region Properties

        public string Name
        {
            get;
            set;
        } = string.Empty;

        public ContainerControl Parent
        {
            get => m_Parent;
            set
            {
                if (m_Parent == value)
                {
                    return;
                }

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
                {
                    m_Parent.AddChildInternal(this);
                }

                CacheRootHandle();
                OnParentChangedInternal();
                if (m_Parent != null && oldParentSize != m_Parent.Size)
                {
                    OnParentResized();
                }
            }
        }

        public bool HasParent => m_Parent != null;

        public int IndexInParent
        {
            get
            {
                ContainerControl parent = m_Parent;
                if (parent == null)
                {
                    return -1;
                }
                return parent.GetChildIndex(this);
            }
            set => m_Parent!.ChangeChildIndex(this, value);
        }

        public AnchorPresets AnchorPreset
        {
            get
            {
                for (int i = 0; i < anchorPresetsData.Length; i++)
                {
                    if (Float2.NearEqual(m_AnchorMin, anchorPresetsData[i].Min) &&
                        Float2.NearEqual(m_AnchorMax, anchorPresetsData[i].Max))
                    {
                        return anchorPresetsData[i].Preset;
                    }
                }
                return AnchorPresets.Custom;
            }
            set => SetAnchorPreset(value, false);
        }

        public bool Enabled
        {
            get => m_Enabled;
            set
            {
                if (m_Enabled != value)
                {
                    m_Enabled = value;
                    if (!value)
                    {
                        ClearState();
                    }
                }
            }
        }

        public bool EnabledInHierarchy
        {
            get
            {
                if (!m_Enabled)
                {
                    return false;
                }
                ContainerControl parent = m_Parent;
                if (parent == null)
                {
                    return true;
                }
                return parent.EnabledInHierarchy;
            }
        }

        public bool Visible
        {
            get => m_Visible;
            set
            {
                if (m_Visible == value)
                {
                    return;
                }

                m_Visible = value;
                if (!value)
                {
                    ClearState();
                }

                OnVisibleChanged();
                m_Parent?.PerformLayout();
            }
        }

        public bool VisibleInHierarchy
        {
            get
            {
                if (!m_Visible)
                {
                    return false;
                }
                ContainerControl parent = m_Parent;
                if (parent == null)
                {
                    return true;
                }
                return parent.VisibleInHierarchy;
            }
        }

        public bool IsDisposing => m_IsDisposing;

        public bool IsDisposed => m_IsDisposing;

        public virtual RootControl Root => m_Root;

        public virtual WindowRootControl RootWindow()
        {
            return m_Parent?.RootWindow();
        }

        public float GetDpiScale()
        {
            WindowRootControl rootWindow = RootWindow();
            if (rootWindow != null)
            {
                return rootWindow.Window.DpiScale;
            }
            return Platform.DpiScale;
        }

        public Float2 ScreenPos
        {
            get
            {
                RootControl parentWindow = Root!;
                SE.Debug.Assert(parentWindow != null, "Missing parent window.");
                Float2 clientPosition = PointToWindow(Float2.Zero);
                return parentWindow!.PointToScreen(clientPosition);
            }
        }

        public Matrix3x3 GetCachedTransform()
        {
            return m_CachedTransform;
        }

        public Matrix3x3 CachedTransform => m_CachedTransform;

        #endregion

        public virtual CursorType Cursor
        {
            get
            {
                ContainerControl parent = Parent;
                if (parent == null)
                {
                    return CursorType.Default;
                }
                return parent.Cursor;
            }
            set
            {
                if (Parent != null)
                {
                    Parent.Cursor = value;
                }
            }
        }

        public object Tag;

        public Control() : this(Rectangle.Empty)
        {
        }

        public Control(float x, float y, float width, float height) : this(new Rectangle(x, y, width, height))
        {
        }

        public Control(Float2 location, Float2 size) : this(new Rectangle(location, size))
        {
        }

        public Control(Rectangle bounds)
        {
            m_Bounds = bounds;
            m_Offsets = new Margin(bounds.X, bounds.Width, bounds.Y, bounds.Height);
            UpdateTransform();
            m_OnUpdateTooltip = OnUpdateTooltip;
        }

        public void Dispose()
        {
            if (m_IsDisposing)
            {
                return;
            }
            OnDestroy();
            m_Parent = null;
            m_OnUpdateTooltip = null;
        }

        public virtual void Update(float deltaTime)
        {
        }

        public void OnUpdateTooltip(float deltaTime)
        {
            // Tooltip.OnMouseOverControl(this, deltaTime);
        }

        public virtual void Draw()
        {
            if (BackgroundColor.A > 0.0f)
            {
                Render2D.FillRectangle(new Rectangle(Float2.Zero, Size), BackgroundColor);
            }
        }

        public virtual void PerformLayout(bool force = false)
        {
            _ = force;
        }

        public virtual void ClearState()
        {
            // ClearState deliberately follows native ordering; do not pre-clear the flags.
            Defocus();
            if (m_IsMouseOver)
            {
                OnMouseLeave();
            }

            if (m_IsDragOver)
            {
                OnDragLeave();
            }

            while (m_TouchOvers.Count != 0)
            {
                OnTouchLeave(m_TouchOvers[0]);
            }
        }

        #region Bounds

        public float X
        {
            get => m_Bounds.X;
            set => SetBounds(value, Y, Width, Height);
        }

        public float Y
        {
            get => m_Bounds.Y;
            set => SetBounds(X, value, Width, Height);
        }

        public float LocalX
        {
            get => LocalLocation.X;
            set => LocalLocation = new Float2(value, LocalLocation.Y);
        }

        public float LocalY
        {
            get => LocalLocation.Y;
            set => LocalLocation = new Float2(LocalLocation.X, value);
        }

        public Float2 AnchorMin
        {
            get => m_AnchorMin;
            set
            {
                if (m_AnchorMin != value)
                {
                    Rectangle bounds = m_Bounds;
                    m_AnchorMin = value;
                    UpdateBounds();
                    SetBounds(bounds);
                }
            }
        }

        public Float2 AnchorMax
        {
            get => m_AnchorMax;
            set
            {
                if (m_AnchorMax != value)
                {
                    Rectangle bounds = m_Bounds;
                    m_AnchorMax = value;
                    UpdateBounds();
                    SetBounds(bounds);
                }
            }
        }

        public Margin Offsets
        {
            get => m_Offsets;
            set
            {
                if (m_Offsets != value)
                {
                    m_Offsets = value;
                    UpdateBounds();
                }
            }
        }

        public Float2 Location
        {
            get => LocalLocation;
            set
            {
                if (m_Bounds.Location == value)
                {
                    return;
                }
                Bounds = new Rectangle(value, m_Bounds.Size);
            }
        }

        public Float2 LocalLocation
        {
            get
            {
                Float2 localLocation = m_Bounds.Location;
                if (m_Parent != null)
                {
                    _ = localLocation - (m_Parent.m_Bounds.Size * (m_AnchorMax + m_AnchorMin) * 0.5f);
                }

                localLocation += Size * m_Pivot;
                return localLocation;
            }
            set
            {
                if (m_Parent != null)
                {
                    SetBounds(new Rectangle(
                        value + (m_Parent.Bounds.Size * (m_AnchorMax + m_AnchorMin) * 0.5f) - Size * m_Pivot,
                        Size));
                }
                else
                {
                    SetBounds(new Rectangle(value - Size * m_Pivot, Size));
                }
            }
        }

        public bool PivotRelative
        {
            get => m_PivotRelativeSizing;
            set => m_PivotRelativeSizing = value;
        }

        public float Width
        {
            get => m_Bounds.Width;
            set
            {
                if (Mathf.NearEqual(Width, value))
                {
                    return;
                }

                Rectangle bounds = new(X, Y, value, Height);
                if (m_PivotRelativeSizing)
                {
                    bounds.X += (Width - value) * m_Pivot.X;
                }

                SetBounds(bounds);
            }
        }

        public float Height
        {
            get => m_Bounds.Height;
            set
            {
                if (Mathf.NearEqual(Height, value))
                {
                    return;
                }

                Rectangle bounds = new(X, Y, Width, value);
                if (m_PivotRelativeSizing)
                {
                    bounds.Y += (Height - value) * m_Pivot.Y;
                }

                SetBounds(bounds);
            }
        }

        public Float2 Size
        {
            get => m_Bounds.Size;
            set
            {
                if (m_Bounds.Size == value)
                {
                    return;
                }
                Bounds = new Rectangle(m_Bounds.Location, value);
            }
        }

        public float Top => m_Bounds.Top;

        public float Bottom => m_Bounds.Bottom;

        public float Left => m_Bounds.Left;

        public float Right => m_Bounds.Right;

        public Float2 UpperLeft => m_Bounds.UpperLeft;

        public Float2 UpperRight => m_Bounds.UpperRight;

        public Float2 BottomRight => m_Bounds.BottomRight;

        public Float2 BottomLeft => m_Bounds.BottomLeft;

        public Float2 Center
        {
            get => m_Bounds.Center;
            set => Location = value - Size * 0.5f;
        }

        public Rectangle Bounds
        {
            get => m_Bounds;
            set => SetBounds(value);
        }

        public Float2 Scale
        {
            get => m_Scale;
            set
            {
                if (m_Scale != value)
                {
                    SetScaleInternal(value);
                }
            }
        }

        public Float2 Pivot
        {
            get => m_Pivot;
            set
            {
                if (m_Pivot != value)
                {
                    SetPivotInternal(value);
                }
            }
        }

        public Float2 Shear
        {
            get => m_Shear;
            set
            {
                if (m_Shear != value)
                {
                    SetShearInternal(value);
                }
            }
        }

        public float Rotation
        {
            get => m_Rotation;
            set
            {
                if (!Mathf.NearEqual(m_Rotation, value))
                {
                    SetRotationInternal(value);
                }
            }
        }

        public void UpdateBounds()
        {
            Rectangle previousBounds = m_Bounds;
            Margin anchors;
            Float2 offset;
            if (m_Parent != null)
            {
                Rectangle parentBounds = m_Parent.GetDesireClientArea();
                anchors = new Margin(
                    m_AnchorMin.X * parentBounds.Size.X,
                    m_AnchorMax.X * parentBounds.Size.X,
                    m_AnchorMin.Y * parentBounds.Size.Y,
                    m_AnchorMax.Y * parentBounds.Size.Y);
                offset = parentBounds.Location;
            }
            else
            {
                anchors = Margin.Zero;
                offset = Float2.Zero;
            }

            m_Bounds.X = anchors.Left + m_Offsets.Left;
            if (m_AnchorMin.X != m_AnchorMax.X)
            {
                m_Bounds.Width = anchors.Right - m_Bounds.X - m_Offsets.Right;
            }
            else
            {
                m_Bounds.Width = m_Offsets.Right;
            }

            m_Bounds.Y = anchors.Top + m_Offsets.Top;
            if (m_AnchorMin.Y != m_AnchorMax.Y)
            {
                m_Bounds.Height = anchors.Bottom - m_Bounds.Y - m_Offsets.Bottom;
            }
            else
            {
                m_Bounds.Height = m_Offsets.Bottom;
            }

            m_Bounds.Location += offset;
            UpdateTransform();
            if (m_Bounds.Location != previousBounds.Location)
            {
                OnLocationChanged();
            }
            if (m_Bounds.Size != previousBounds.Size)
            {
                OnSizeChanged();
            }
        }

        public void UpdateTransform()
        {
            // Keep the native scale/shear/rotation order and pivot translation intact.
            Float2 pivot = m_Pivot * m_Bounds.Size;
            Float2 negativePivot = -pivot;
            pivot += m_Bounds.Location;
            float shearX = m_Shear.X == 0
                ? 0
                : 1.0f / Mathf.Tan(Mathf.DegreesToRadians * (90.0f - Mathf.Clamp(m_Shear.X, -89.0f, 89.0f)));
            float shearY = m_Shear.Y == 0
                ? 0
                : 1.0f / Mathf.Tan(Mathf.DegreesToRadians * (90.0f - Mathf.Clamp(m_Shear.Y, -89.0f, 89.0f)));
            Matrix3x3 transform = new(m_Scale.X, m_Scale.X * shearY, 0, m_Scale.Y * shearX, m_Scale.Y, 0, 0, 0, 1);
            float sin = Mathf.Sin(Mathf.DegreesToRadians * m_Rotation);
            float cos = Mathf.Cos(Mathf.DegreesToRadians * m_Rotation);
            transform.M11 = (m_Scale.X * cos) + (transform.M12 * -sin);
            transform.M12 = (m_Scale.X * sin) + (transform.M12 * cos);
            float m21 = (transform.M21 * cos) + (m_Scale.Y * -sin);
            transform.M22 = (transform.M21 * sin) + (m_Scale.Y * cos);
            transform.M21 = m21;
            transform.M31 = (negativePivot.X * transform.M11) + (negativePivot.Y * transform.M21) + pivot.X;
            transform.M32 = (negativePivot.X * transform.M12) + (negativePivot.Y * transform.M22) + pivot.Y;
            m_CachedTransform = transform;
            Matrix3x3.Invert(ref m_CachedTransform, out m_CachedTransformInv);
        }

        public void SetAnchorPreset(AnchorPresets anchorPreset, bool preserveBounds, bool setPivotToo = false)
        {
            for (int i = 0; i < anchorPresetsData.Length; i++)
            {
                if (anchorPresetsData[i].Preset != anchorPreset)
                {
                    continue;
                }

                Float2 anchorMin = anchorPresetsData[i].Min;
                Float2 anchorMax = anchorPresetsData[i].Max;
                Rectangle bounds = m_Bounds;
                if (!Float2.NearEqual(m_AnchorMin, anchorMin) || !Float2.NearEqual(m_AnchorMax, anchorMax))
                {
                    if (!anchorMin.IsZero || !anchorMax.IsZero)
                    {
                        IsScrollable = false;
                    }

                    m_AnchorMin = anchorMin;
                    m_AnchorMax = anchorMax;
                    if (preserveBounds)
                    {
                        UpdateBounds();
                        SetBounds(bounds);
                    }
                }
                if (!preserveBounds)
                {
                    if (m_Parent != null)
                    {
                        Rectangle parentBounds = m_Parent.GetDesireClientArea();
                        switch (anchorPreset)
                        {
                            case AnchorPresets.TopLeft:
                                bounds.Location = Float2.Zero;
                                break;
                            case AnchorPresets.TopCenter:
                                bounds.Location = new Float2(parentBounds.Left * 0.5f - bounds.Left * 0.5f, 0);
                                break;
                            case AnchorPresets.TopRight:
                                bounds.Location = new Float2(parentBounds.Left - bounds.Left, 0);
                                break;
                            case AnchorPresets.MiddleLeft:
                                bounds.Location = new Float2(0, parentBounds.Height * 0.5f - bounds.Height * 0.5f);
                                break;
                            case AnchorPresets.MiddleCenter:
                                bounds.Location = new Float2(
                                    parentBounds.Left * 0.5f - bounds.Left * 0.5f,
                                    parentBounds.Height * 0.5f - bounds.Height * 0.5f);
                                break;
                            case AnchorPresets.MiddleRight:
                                bounds.Location = new Float2(
                                    parentBounds.Left - bounds.Left,
                                    parentBounds.Height * 0.5f - bounds.Height * 0.5f);
                                break;
                            case AnchorPresets.BottomLeft:
                                bounds.Location = new Float2(0, parentBounds.Height - bounds.Height);
                                break;
                            case AnchorPresets.BottomCenter:
                                bounds.Location = new Float2(
                                    parentBounds.Left * 0.5f - bounds.Left * 0.5f,
                                    parentBounds.Height - bounds.Height);
                                break;
                            case AnchorPresets.BottomRight:
                                bounds.Location = new Float2(
                                    parentBounds.Left - bounds.Left,
                                    parentBounds.Height - bounds.Height);
                                break;
                            case AnchorPresets.VerticalStretchLeft:
                                bounds.Location = Float2.Zero;
                                bounds.Size = new Float2(bounds.Left, parentBounds.Height);
                                break;
                            case AnchorPresets.VerticalStretchCenter:
                                bounds.Location = new Float2(parentBounds.Left * 0.5f - bounds.Left * 0.5f, 0);
                                bounds.Size = new Float2(bounds.Left, parentBounds.Height);
                                break;
                            case AnchorPresets.VerticalStretchRight:
                                bounds.Location = new Float2(parentBounds.Left - bounds.Left, 0);
                                bounds.Size = new Float2(bounds.Left, parentBounds.Height);
                                break;
                            case AnchorPresets.HorizontalStretchTop:
                                bounds.Location = Float2.Zero;
                                bounds.Size = new Float2(parentBounds.Left, bounds.Height);
                                break;
                            case AnchorPresets.HorizontalStretchMiddle:
                                bounds.Location = new Float2(0, parentBounds.Height * 0.5f - bounds.Height * 0.5f);
                                bounds.Size = new Float2(parentBounds.Left, bounds.Height);
                                break;
                            case AnchorPresets.HorizontalStretchBottom:
                                bounds.Location = new Float2(0, parentBounds.Height - bounds.Height);
                                bounds.Size = new Float2(parentBounds.Left, bounds.Height);
                                break;
                            case AnchorPresets.StretchAll:
                                bounds.Location = Float2.Zero;
                                bounds.Size = parentBounds.Size;
                                break;
                        }
                        bounds.Location += parentBounds.Location;
                    }
                    SetBounds(bounds);
                }
                if (setPivotToo)
                {
                    Pivot = (anchorMin + anchorMax) / 2.0f;
                }

                m_Parent?.PerformLayout();
                return;
            }
        }

        public void SetBounds(float x, float y, float width, float height)
        {
            SetBounds(new Rectangle(x, y, width, height));
        }

        public void SetBounds(Rectangle bounds)
        {
            if (m_Bounds == bounds)
            {
                return;
            }
            // Native bounds assignment first derives offsets from the parent client area.
            Margin anchors;
            if (m_Parent != null)
            {
                Rectangle parentBounds = m_Parent.GetDesireClientArea();
                anchors = new Margin(
                    m_AnchorMin.X * parentBounds.Size.X + parentBounds.Location.X,
                    m_AnchorMax.X * parentBounds.Size.X,
                    m_AnchorMin.Y * parentBounds.Size.Y + parentBounds.Location.Y,
                    m_AnchorMax.Y * parentBounds.Size.Y);
            }
            else
            {
                anchors = Margin.Zero;
            }

            m_Offsets.Left = bounds.X - anchors.Left;
            if (m_AnchorMin.X != m_AnchorMax.X)
            {
                m_Offsets.Right = anchors.Right - bounds.X - bounds.Width;
            }
            else
            {
                m_Offsets.Right = bounds.Width;
            }

            m_Offsets.Top = bounds.Y - anchors.Top;
            if (m_AnchorMin.Y != m_AnchorMax.Y)
            {
                m_Offsets.Bottom = anchors.Bottom - bounds.Y - bounds.Height;
            }
            else
            {
                m_Offsets.Bottom = bounds.Height;
            }

            UpdateBounds();
        }

        #endregion

        #region Focus

        public virtual bool ContainsFocus => m_IsFocused;

        public bool IsFocused => m_IsFocused;

        public bool IsNavFocused => m_IsNavFocused;

        public virtual void Focus()
        {
            if (!IsFocused)
            {
                Focus(this);
            }
        }

        protected virtual bool Focus(Control control)
        {
            return m_Parent != null && m_Parent.Focus(control);
        }

        public virtual void Defocus()
        {
            if (ContainsFocus)
            {
                Focus(null);
            }
        }

        public virtual void OnGetFocus()
        {
            m_IsFocused = true;
            m_IsNavFocused = false;
        }

        public virtual void OnLostFocus()
        {
            m_IsFocused = false;
            m_IsNavFocused = false;
        }

        public virtual void OnStartContainsFocus()
        {
        }

        public virtual void OnEndContainsFocus()
        {
        }

        public void StartMouseCapture(bool useMouseScreenOffset = false)
        {
            Root?.StartTrackingMouse(this, useMouseScreenOffset);
        }

        public void EndMouseCapture()
        {
            Root?.EndTrackingMouse();
        }

        public virtual void OnEndMouseCapture()
        {
        }

        #endregion

        #region Navigation

        public Control NavTargetUp
        {
            get;
            set;
        }

        public Control NavTargetDown
        {
            get;
            set;
        }

        public Control NavTargetLeft
        {
            get;
            set;
        }

        public Control NavTargetRight
        {
            get;
            set;
        }

        public virtual Control GetNavTarget(NavDirection direction)
        {
            return direction switch
            {
                NavDirection.Up => NavTargetUp,
                NavDirection.Down => NavTargetDown,
                NavDirection.Left => NavTargetLeft,
                NavDirection.Right => NavTargetRight,
                _ => null,
            };
        }

        public virtual Float2 GetNavOrigin(NavDirection direction)
        {
            return direction switch
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
        }

        public virtual Control OnNavigate(NavDirection direction, Float2 location, Control caller,
            List<Control> visited)
        {
            if (caller == m_Parent && AutoFocus && Visible)
            {
                return this;
            }

            return m_Parent?.OnNavigate(direction, PointToParent(GetNavOrigin(direction)), caller, visited);
        }

        public virtual void NavigationFocus()
        {
            Focus();
            if (!IsFocused)
            {
                return;
            }

            m_IsNavFocused = true;
            ContainerControl parent = Parent;
            while (parent != null)
            {
                if (parent is Panel panel &&
                    ((panel.vScrollBar != null && panel.vScrollBar.Enabled) ||
                     (panel.hScrollBar != null && panel.hScrollBar.Enabled)))
                {
                    panel.ScrollViewTo(this);
                    break;
                }
                parent = parent.Parent;
            }
        }

        public virtual void OnSubmit()
        {
        }

        #endregion

        #region Mouse

        public virtual bool IsMouseOver => m_IsMouseOver;

        public virtual void OnMouseEnter(Float2 location)
        {
            m_IsMouseOver = true;
            if (ShowTooltip() && OnTestTooltipOverControl(location))
            {
                // Tooltip.OnMouseEnterControl(this);
                SetUpdate(m_TooltipUpdate, m_OnUpdateTooltip);
            }
        }

        public virtual void OnMouseMove(Float2 location)
        {
            if (ShowTooltip() && OnTestTooltipOverControl(location))
            {
                if (m_TooltipUpdate != null)
                {
                    // Tooltip.OnMouseEnterControl(this);
                    SetUpdate(m_TooltipUpdate, m_OnUpdateTooltip);
                }
            }
            else if (m_TooltipUpdate != null)
            {
                SetUpdate(m_TooltipUpdate, null);
                // Tooltip.OnMouseLeaveControl(this);
            }
        }

        public virtual void OnMouseLeave()
        {
            m_IsMouseOver = false;
            if (m_TooltipUpdate != null)
            {
                SetUpdate(m_TooltipUpdate, null);
                // Tooltip.OnMouseLeaveControl(this);
            }
        }

        public virtual bool OnMouseWheel(Float2 location, float delta)
        {
            return false;
        }

        public virtual bool OnMouseDown(Float2 location, MouseButton button)
        {
            return false;
        }

        public virtual bool OnMouseUp(Float2 location, MouseButton button)
        {
            return false;
        }

        public virtual bool OnMouseDoubleClick(Float2 location, MouseButton button)
        {
            return false;
        }

        #endregion

        #region Keyboard

        public virtual bool OnCharInput(char character)
        {
            return false;
        }

        public virtual bool OnKeyDown(KeyboardKeys key)
        {
            return false;
        }

        public virtual void OnKeyUp(KeyboardKeys key)
        {
        }

        #endregion

        #region Touch

        public virtual bool IsTouchOver()
        {
            return m_TouchOvers.Count != 0;
        }

        public virtual bool IsTouchPointerOver(int pointerIndex)
        {
            return m_TouchOvers.Contains(pointerIndex);
        }

        public virtual void OnTouchEnter(Float2 location, int pointerIndex)
        {
            m_TouchOvers.Add(pointerIndex);
        }

        public virtual bool OnTouchDown(Float2 location, int pointerIndex)
        {
            return false;
        }

        public virtual void OnTouchMove(Float2 location, int pointerIndex)
        {
        }

        public virtual bool OnTouchUp(Float2 location, int pointerIndex)
        {
            return false;
        }

        public virtual void OnTouchLeave(int pointerIndex)
        {
            m_TouchOvers.Remove(pointerIndex);
            if (m_TouchOvers.Count == 0)
            {
                OnTouchLeave();
            }
        }

        public virtual void OnTouchLeave()
        {
        }

        #endregion

        #region Drag&Drop

        public virtual bool IsDragOver()
        {
            return m_IsDragOver;
        }

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

        public virtual void OnDragLeave()
        {
            m_IsDragOver = false;
        }

        public virtual void DoDragDrop(DragData data)
        {
            Root?.DoDragDrop(data);
        }

        #endregion

        #region Tooltip

        public Tooltip GetTooltip()
        {
            if (Tooltip != null)
            {
                return Tooltip;
            }
            return Style.Current.SharedTooltip;
        }

        protected virtual bool ShowTooltip()
        {
            return !string.IsNullOrEmpty(TooltipText) && TooltipText.Length > 0;
        }

        public Control LinkTooltip(string text, Tooltip customTooltip = null)
        {
            TooltipText = text;
            Tooltip = customTooltip;
            return this;
        }

        public void UnlinkTooltip()
        {
            TooltipText = string.Empty;
            Tooltip = null;
        }

        public virtual bool OnShowTooltip(string text, out Float2 location, out Rectangle area)
        {
            text = TooltipText;
            location = new Float2(0.5f, 1) * Size;
            area = new Rectangle(Float2.Zero, Size);
            return ShowTooltip();
        }

        public virtual void OnTooltipShown(Tooltip tooltip)
        {
        }

        public virtual bool OnTestTooltipOverControl(Float2 location)
        {
            return ContainsPoint(location) && ShowTooltip();
        }

        #endregion

        #region Helper Functions

        public virtual bool RayCast(ref Float2 location, out Control hit)
        {
            if (ContainsPoint(location, true))
            {
                hit = this;
                return true;
            }
            hit = null;
            return false;
        }

        public virtual bool IntersectsContent(Float2 locationParent, out Float2 location)
        {
            location = PointFromParent(locationParent);
            return ContainsPoint(location);
        }

        public virtual bool ContainsPoint(Float2 location, bool precise = false)
        {
            _ = precise;
            return location.X >= 0 && location.Y >= 0 && location.X <= Width && location.Y <= Height;
        }

        public virtual Float2 PointToParent(Float2 location)
        {
            Matrix3x3.Transform2D(ref location, ref m_CachedTransform, out Float2 result);
            return result;
        }

        public virtual Float2 PointToParent(ref Float2 location)
        {
            return PointToParent(location);
        }

        public Float2 PointToParent(ContainerControl parent, Float2 location)
        {
            ArgumentNullException.ThrowIfNull(parent);
            Control control = this;
            while (control != null)
            {
                location = control.PointToParent(location);
                control = control.Parent;
                if (control == parent)
                {
                    break;
                }
            }
            return location;
        }

        public virtual Float2 PointFromParent(Float2 location)
        {
            Matrix3x3.Transform2D(ref location, ref m_CachedTransformInv, out Float2 result);
            return result;
        }

        public virtual Float2 PointFromParent(ref Float2 location)
        {
            return PointFromParent(location);
        }

        public Float2 PointFromParent(ContainerControl parent, Float2 location)
        {
            ArgumentNullException.ThrowIfNull(parent);
            List<Control> path = new();
            Control control = this;
            while (control != null && control != parent)
            {
                path.Add(control);
                control = control.Parent;
            }
            for (int i = path.Count - 1; i >= 0; i--)
            {
                location = path[i].PointFromParent(location);
            }

            return location;
        }

        public Float2 PointToWindow(Float2 location)
        {
            location = PointToParent(location);
            return m_Parent == null ? location : m_Parent.PointToWindow(location);
        }

        public Float2 PointFromWindow(Float2 location)
        {
            if (m_Parent != null)
            {
                location = m_Parent.PointFromWindow(location);
            }

            return PointFromParent(location);
        }

        public virtual Float2 PointToScreen(Float2 location)
        {
            location = PointToParent(location);
            return m_Parent == null ? location : m_Parent.PointToScreen(location);
        }

        public virtual Float2 PointFromScreen(Float2 location)
        {
            if (m_Parent != null)
            {
                location = m_Parent.PointFromScreen(location);
            }

            return PointFromParent(location);
        }

        public Float2 PointToRoot(Float2 location)
        {
            location = PointToParent(location);
            return m_Parent == null ? location : m_Parent.PointToRoot(location);
        }

        public Float2 PointFromRoot(Float2 location)
        {
            if (m_Parent != null)
            {
                location = m_Parent.PointFromRoot(location);
            }

            return PointFromParent(location);
        }

        public Rectangle ScreenBounds
        {
            get
            {
                Float2 upperLeft = PointToRoot(Float2.Zero);
                Float2 bottomRight = PointToRoot(Size);
                return new Rectangle(upperLeft, bottomRight - upperLeft);
            }
        }

        #endregion

        #region Control Action

        protected virtual void OnLocationChanged()
        {
            LocationChanged?.Invoke(this);
        }

        protected virtual void OnSizeChanged()
        {
            SizeChanged?.Invoke(this);
            m_Parent?.OnChildResized(this);
        }

        protected virtual void SetScaleInternal(Float2 value)
        {
            m_Scale = value;
            UpdateTransform();
            m_Parent?.OnChildResized(this);
        }

        protected virtual void SetPivotInternal(Float2 value)
        {
            m_Pivot = value;
            UpdateTransform();
            m_Parent?.OnChildResized(this);
        }

        protected virtual void SetShearInternal(Float2 value)
        {
            m_Shear = value;
            UpdateTransform();
            m_Parent?.OnChildResized(this);
        }

        protected virtual void SetRotationInternal(float value)
        {
            m_Rotation = value;
            UpdateTransform();
            m_Parent?.OnChildResized(this);
        }

        protected virtual void OnVisibleChanged()
        {
            if (!m_Visible && m_IsMouseOver)
            {
                OnMouseLeave();
            }
            VisibleChanged?.Invoke(this);
        }

        protected virtual void OnParentChangedInternal()
        {
            ParentChanged?.Invoke(this);
        }

        protected virtual void AddUpdateCallbacks(RootControl root)
        {
            if (m_TooltipUpdate != null)
            {
                root.UpdateCallbacksToAdd.Add(m_TooltipUpdate);
            }
        }

        protected virtual void RemoveUpdateCallbacks(RootControl root)
        {
            if (m_TooltipUpdate != null)
            {
                root.UpdateCallbacksToRemove.Add(m_TooltipUpdate);
            }
        }

        protected void SetUpdate(Action<float> onUpdate, Action<float> value)
        {
            // Control.cpp passes the delegate pointer by value; assignment only changes this local.
            if ((object)onUpdate == (object)value)
            {
                return;
            }

            if (m_Root != null && onUpdate != null)
            {
                m_Root.UpdateCallbacksToRemove.Add(onUpdate);
            }

            onUpdate = value;
            if (m_Root != null && onUpdate != null)
            {
                m_Root.UpdateCallbacksToAdd.Add(onUpdate);
            }
        }

        public virtual void CacheRootHandle()
        {
            if (m_Root != null)
            {
                RemoveUpdateCallbacks(m_Root);
            }

            if (m_Parent != null)
            {
                m_Root = m_Parent.Root;
            }

            if (m_Root != null)
            {
                AddUpdateCallbacks(m_Root);
            }
        }

        public virtual void OnParentResized()
        {
            if (!m_AnchorMin.IsZero || !m_AnchorMax.IsZero)
            {
                UpdateBounds();
            }
        }

        public virtual void OnDestroy()
        {
            m_IsDisposing = true;
            Defocus();
            UnlinkTooltip();
            Tag = null;
        }

        #endregion


        public virtual int CompareTo(Control other)
        {
            return (int)(Y - other!.Y);
        }
    }
}
