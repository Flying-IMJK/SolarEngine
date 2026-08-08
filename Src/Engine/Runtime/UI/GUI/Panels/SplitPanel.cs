using System;

namespace SE.GUI
{
    /// <summary>
    /// Contains two panels separated by a draggable splitter.
    /// </summary>
    public class SplitPanel : ContainerControl
    {
        public const int SplitterSize = 4;

        private Orientation m_Orientation;
        private float m_SplitterValue = 0.5f;
        private Rectangle m_SplitterBounds;
        private bool m_IsTrackingSplitter;
        private bool m_IsMouseOverSplitter;

        public SplitPanel(Orientation orientation = Orientation.Horizontal)
        {
            AutoFocus = false;
            m_Orientation = orientation;
            Panel1 = new Panel();
            Panel2 = new Panel();
            AddChild(Panel1);
            AddChild(Panel2);
            UpdateSplitterBounds();
        }

        public Panel Panel1 { get; }
        public Panel Panel2 { get; }

        public Orientation Orientation
        {
            get => m_Orientation;
            set
            {
                if (m_Orientation == value)
                    return;
                m_Orientation = value;
                PerformLayout();
            }
        }

        public float SplitterValue
        {
            get => m_SplitterValue;
            set
            {
                float clamped = Math.Clamp(value, 0.0f, 1.0f);
                if (MathF.Abs(m_SplitterValue - clamped) <= float.Epsilon)
                    return;
                m_SplitterValue = clamped;
                PerformLayout();
            }
        }

        public Rectangle SplitterBounds => m_SplitterBounds;

        public override bool OnMouseDown(Float2 location, MouseButton button)
        {
            if (button != MouseButton.Left || !m_SplitterBounds.Contains(location))
                return false;

            m_IsTrackingSplitter = true;
            Root?.StartTrackingMouse(this);
            return true;
        }

        public override void OnMouseMove(Float2 location)
        {
            m_IsMouseOverSplitter = m_SplitterBounds.Contains(location);
            if (m_IsTrackingSplitter)
                SplitterValue = m_Orientation == Orientation.Horizontal ? location.X / MathF.Max(1.0f, Width) : location.Y / MathF.Max(1.0f, Height);
        }

        public override bool OnMouseUp(Float2 location, MouseButton button)
        {
            if (button != MouseButton.Left || !m_IsTrackingSplitter)
                return false;

            m_IsTrackingSplitter = false;
            Root?.EndTrackingMouse();
            return true;
        }

        public override void ClearState()
        {
            m_IsTrackingSplitter = false;
            m_IsMouseOverSplitter = false;
            base.ClearState();
        }

        public override void Draw()
        {
            base.Draw();
            if (!VisibleInHierarchy || IsDisposed)
                return;

            Rectangle splitter = new Rectangle(ScreenPos + m_SplitterBounds.Location, m_SplitterBounds.Size);
            Color color = m_IsTrackingSplitter ? Style.Current.BackgroundSelected : m_IsMouseOverSplitter ? Style.Current.BackgroundHighlighted : Style.Current.BackgroundNormal;
            Render2D.FillRectangle(ref splitter, ref color);
        }

        protected override void OnLayoutChildren()
        {
            UpdateSplitterBounds();
            float half = SplitterSize * 0.5f;
            if (m_Orientation == Orientation.Horizontal)
            {
                float split = MathF.Round(m_SplitterValue * Width);
                Panel1.SetBounds(0.0f, 0.0f, MathF.Max(0.0f, split - half), Height);
                Panel2.SetBounds(MathF.Min(Width, split + half), 0.0f, MathF.Max(0.0f, Width - split - half), Height);
            }
            else
            {
                float split = MathF.Round(m_SplitterValue * Height);
                Panel1.SetBounds(0.0f, 0.0f, Width, MathF.Max(0.0f, split - half));
                Panel2.SetBounds(0.0f, MathF.Min(Height, split + half), Width, MathF.Max(0.0f, Height - split - half));
            }
        }

        private void UpdateSplitterBounds()
        {
            if (m_Orientation == Orientation.Horizontal)
            {
                float split = MathF.Round(m_SplitterValue * Width);
                m_SplitterBounds = new Rectangle(Math.Clamp(split - SplitterSize * 0.5f, 0.0f, Width), 0.0f, SplitterSize, Height);
            }
            else
            {
                float split = MathF.Round(m_SplitterValue * Height);
                m_SplitterBounds = new Rectangle(0.0f, Math.Clamp(split - SplitterSize * 0.5f, 0.0f, Height), Width, SplitterSize);
            }
        }
    }
}
