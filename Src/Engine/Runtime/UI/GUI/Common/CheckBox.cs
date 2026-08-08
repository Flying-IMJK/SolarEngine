using System;

namespace SE.GUI
{
    /// <summary>
    /// The state of a <see cref="CheckBox"/>.
    /// </summary>
    public enum CheckBoxState
    {
        Default,
        Checked,
        Intermediate,
    }

    /// <summary>
    /// A managed three-state checkbox.
    /// </summary>
    public class CheckBox : Control
    {
        private CheckBoxState m_State;
        private bool m_IsPressed;
        private bool m_IsMouseOverBox;
        private float m_BoxSize;
        private Rectangle m_Box;

        public CheckBox()
            : this(0.0f, 0.0f)
        {
        }

        public CheckBox(float x, float y, bool isChecked = false, float size = 18.0f)
            : base(new Rectangle(x, y, size, size))
        {
            m_State = isChecked ? CheckBoxState.Checked : CheckBoxState.Default;
            m_BoxSize = MathF.Min(16.0f, size);
            BorderColor = Style.Current.Foreground;
            BorderColorHighlighted = Style.Current.BackgroundHighlighted;
            ImageColor = Style.Current.Foreground;
            CacheBox();
        }

        public event Action<CheckBox>? StateChanged;

        public CheckBoxState State
        {
            get => m_State;
            set
            {
                if (m_State == value)
                    return;

                m_State = value;
                StateChanged?.Invoke(this);
            }
        }

        public bool Checked
        {
            get => State == CheckBoxState.Checked;
            set => State = value ? CheckBoxState.Checked : CheckBoxState.Default;
        }

        public bool Intermediate
        {
            get => State == CheckBoxState.Intermediate;
            set => State = value ? CheckBoxState.Intermediate : CheckBoxState.Default;
        }

        public float BoxSize
        {
            get => m_BoxSize;
            set
            {
                m_BoxSize = MathF.Max(0.0f, value);
                CacheBox();
            }
        }

        public bool HasBorder { get; set; } = true;
        public float BorderThickness { get; set; } = 1.0f;
        public Color BorderColor { get; set; }
        public Color BorderColorHighlighted { get; set; }
        public Color ImageColor { get; set; }
        public bool IsPressed => m_IsPressed;

        /// <summary>
        /// Toggles between the unchecked and checked states.
        /// </summary>
        public void Toggle()
        {
            Checked = !Checked;
        }

        public override void OnMouseMove(Float2 location)
        {
            m_IsMouseOverBox = m_Box.Contains(location);
        }

        public override bool OnMouseDown(Float2 location, MouseButton button)
        {
            if (button != MouseButton.Left || m_IsPressed)
                return false;

            m_IsPressed = true;
            Root?.StartTrackingMouse(this);
            return true;
        }

        public override bool OnMouseUp(Float2 location, MouseButton button)
        {
            if (button != MouseButton.Left || !m_IsPressed)
                return false;

            m_IsPressed = false;
            Root?.EndTrackingMouse();
            if (m_Box.Contains(location))
                Toggle();
            return true;
        }

        public override void ClearState()
        {
            m_IsPressed = false;
            m_IsMouseOverBox = false;
            base.ClearState();
        }

        protected override void OnDraw()
        {
            base.OnDraw();

            Rectangle box = new Rectangle(ScreenPos + m_Box.Location, m_Box.Size);
            Color border = m_IsPressed || m_IsMouseOverBox || IsFocused ? BorderColorHighlighted : BorderColor;
            if (!EnabledInHierarchy)
                border.A *= 0.5f;

            if (HasBorder && BorderThickness > 0.0f && border.A > 0.0f)
                Render2D.DrawRectangle(ref box, ref border, BorderThickness);

            if (State == CheckBoxState.Default)
                return;

            Color image = ImageColor;
            if (!EnabledInHierarchy)
                image.A *= 0.6f;

            if (State == CheckBoxState.Checked)
            {
                Float2 from = new Float2(box.X + box.Width * 0.2f, box.Y + box.Height * 0.55f);
                Float2 middle = new Float2(box.X + box.Width * 0.43f, box.Y + box.Height * 0.76f);
                Float2 to = new Float2(box.X + box.Width * 0.82f, box.Y + box.Height * 0.25f);
                Render2D.DrawLine(ref from, ref middle, ref image, MathF.Max(1.0f, BorderThickness));
                Render2D.DrawLine(ref middle, ref to, ref image, MathF.Max(1.0f, BorderThickness));
            }
            else
            {
                Rectangle mark = new Rectangle(box.X + box.Width * 0.2f, box.Y + box.Height * 0.42f, box.Width * 0.6f, box.Height * 0.16f);
                Render2D.FillRectangle(ref mark, ref image);
            }
        }

        protected override void OnBoundsChanged(bool locationChanged, bool sizeChanged)
        {
            base.OnBoundsChanged(locationChanged, sizeChanged);
            if (sizeChanged)
                CacheBox();
        }

        private void CacheBox()
        {
            m_Box = new Rectangle(0.0f, (Height - m_BoxSize) * 0.5f, m_BoxSize, m_BoxSize);
        }
    }
}
