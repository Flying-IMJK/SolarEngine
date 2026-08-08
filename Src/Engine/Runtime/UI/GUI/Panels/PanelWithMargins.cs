using System;

namespace SE.GUI
{
    /// <summary>
    /// Base panel for layouts that arrange controls within margins.
    /// </summary>
    public class PanelWithMargins : Panel
    {
        private Margin m_Margin = new Margin(2.0f);
        private float m_Spacing = 2.0f;
        private Float2 m_Offset;
        private bool m_AutoSize = true;

        public PanelWithMargins()
            : base(new Rectangle(0.0f, 0.0f, 64.0f, 64.0f))
        {
            AutoFocus = false;
        }

        public PanelWithMargins(Rectangle bounds)
            : this()
        {
            Bounds = bounds;
        }

        public Margin Margin
        {
            get => m_Margin;
            set
            {
                if (m_Margin == value)
                    return;
                m_Margin = value;
                PerformLayout();
            }
        }

        public float LeftMargin
        {
            get => m_Margin.Left;
            set => SetMargin(value, m_Margin.Right, m_Margin.Top, m_Margin.Bottom);
        }

        public float RightMargin
        {
            get => m_Margin.Right;
            set => SetMargin(m_Margin.Left, value, m_Margin.Top, m_Margin.Bottom);
        }

        public float TopMargin
        {
            get => m_Margin.Top;
            set => SetMargin(m_Margin.Left, m_Margin.Right, value, m_Margin.Bottom);
        }

        public float BottomMargin
        {
            get => m_Margin.Bottom;
            set => SetMargin(m_Margin.Left, m_Margin.Right, m_Margin.Top, value);
        }

        public float Spacing
        {
            get => m_Spacing;
            set
            {
                if (m_Spacing == value)
                    return;
                m_Spacing = value;
                PerformLayout();
            }
        }

        public Float2 Offset
        {
            get => m_Offset;
            set
            {
                if (m_Offset == value)
                    return;
                m_Offset = value;
                PerformLayout();
            }
        }

        public bool AutoSize
        {
            get => m_AutoSize;
            set
            {
                if (m_AutoSize == value)
                    return;
                m_AutoSize = value;
                PerformLayout();
            }
        }

        protected Rectangle ContentBounds => new Rectangle(
            m_Margin.Left + m_Offset.X,
            m_Margin.Top + m_Offset.Y,
            MathF.Max(0.0f, Width - m_Margin.Width),
            MathF.Max(0.0f, Height - m_Margin.Height));

        private void SetMargin(float left, float right, float top, float bottom)
        {
            Margin = new Margin(left, right, top, bottom);
        }
    }
}
