using System;

namespace SE.GUI
{
    /// <summary>
    /// Contains two panels separated by a draggable splitter.
    /// </summary>
    public class SplitPanel : ContainerControl
    {
        public const int SplitterSize = 4;

        private Orientation _orientation;
        private float _splitterValue = 0.5f;
        private Rectangle _splitterBounds;
        private bool _isTrackingSplitter;
        private bool _isMouseOverSplitter;

        public SplitPanel(Orientation orientation = Orientation.Horizontal)
        {
            AutoFocus = false;
            _orientation = orientation;
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
            get => _orientation;
            set
            {
                if (_orientation == value)
                    return;
                _orientation = value;
                PerformLayout();
            }
        }

        public float SplitterValue
        {
            get => _splitterValue;
            set
            {
                float clamped = Math.Clamp(value, 0.0f, 1.0f);
                if (MathF.Abs(_splitterValue - clamped) <= float.Epsilon)
                    return;
                _splitterValue = clamped;
                PerformLayout();
            }
        }

        public Rectangle SplitterBounds => _splitterBounds;

        public override bool OnMouseDown(Float2 location, int button)
        {
            if (button != 1 || !_splitterBounds.Contains(location))
                return false;

            _isTrackingSplitter = true;
            Root?.StartTrackingMouse(this);
            return true;
        }

        public override void OnMouseMove(Float2 location)
        {
            _isMouseOverSplitter = _splitterBounds.Contains(location);
            if (_isTrackingSplitter)
                SplitterValue = _orientation == Orientation.Horizontal ? location.X / MathF.Max(1.0f, Width) : location.Y / MathF.Max(1.0f, Height);
        }

        public override bool OnMouseUp(Float2 location, int button)
        {
            if (button != 1 || !_isTrackingSplitter)
                return false;

            _isTrackingSplitter = false;
            Root?.EndTrackingMouse();
            return true;
        }

        public override void ClearState()
        {
            _isTrackingSplitter = false;
            _isMouseOverSplitter = false;
            base.ClearState();
        }

        public override void Draw()
        {
            base.Draw();
            if (!VisibleInHierarchy || IsDisposed)
                return;

            Rectangle splitter = new Rectangle(ScreenPos + _splitterBounds.Location, _splitterBounds.Size);
            Color color = _isTrackingSplitter ? Style.Current.BackgroundSelected : _isMouseOverSplitter ? Style.Current.BackgroundHighlighted : Style.Current.BackgroundNormal;
            Render2D.FillRectangle(ref splitter, ref color);
        }

        protected override void OnLayoutChildren()
        {
            UpdateSplitterBounds();
            float half = SplitterSize * 0.5f;
            if (_orientation == Orientation.Horizontal)
            {
                float split = MathF.Round(_splitterValue * Width);
                Panel1.SetBounds(0.0f, 0.0f, MathF.Max(0.0f, split - half), Height);
                Panel2.SetBounds(MathF.Min(Width, split + half), 0.0f, MathF.Max(0.0f, Width - split - half), Height);
            }
            else
            {
                float split = MathF.Round(_splitterValue * Height);
                Panel1.SetBounds(0.0f, 0.0f, Width, MathF.Max(0.0f, split - half));
                Panel2.SetBounds(0.0f, MathF.Min(Height, split + half), Width, MathF.Max(0.0f, Height - split - half));
            }
        }

        private void UpdateSplitterBounds()
        {
            if (_orientation == Orientation.Horizontal)
            {
                float split = MathF.Round(_splitterValue * Width);
                _splitterBounds = new Rectangle(Math.Clamp(split - SplitterSize * 0.5f, 0.0f, Width), 0.0f, SplitterSize, Height);
            }
            else
            {
                float split = MathF.Round(_splitterValue * Height);
                _splitterBounds = new Rectangle(0.0f, Math.Clamp(split - SplitterSize * 0.5f, 0.0f, Height), Width, SplitterSize);
            }
        }
    }
}
