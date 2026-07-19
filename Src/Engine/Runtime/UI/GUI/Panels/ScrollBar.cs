using System;

namespace SE.GUI
{
    /// <summary>
    /// A compact slider used to expose a scroll position.
    /// </summary>
    public class ScrollBar : Slider
    {
        public ScrollBar(Orientation orientation = Orientation.Vertical)
            : base(orientation == Orientation.Horizontal ? 100.0f : 16.0f, orientation == Orientation.Horizontal ? 16.0f : 100.0f)
        {
            Orientation = orientation;
            Direction = orientation == Orientation.Horizontal ? SliderDirection.HorizontalRight : SliderDirection.VerticalDown;
            Minimum = 0.0f;
            Maximum = 0.0f;
        }

        public Orientation Orientation { get; }
        public float ViewportSize { get; set; }
        public float ContentSize { get; private set; }

        internal override bool ApplyParentChildOffset => false;

        public void SetContentSize(float viewportSize, float contentSize)
        {
            ViewportSize = MathF.Max(0.0f, viewportSize);
            ContentSize = MathF.Max(ViewportSize, contentSize);
            Minimum = 0.0f;
            Maximum = MathF.Max(0.0f, ContentSize - ViewportSize);
            Enabled = Maximum > 0.0f;
            Value = Math.Clamp(Value, Minimum, Maximum);
            float trackLength = Orientation == Orientation.Horizontal ? Width : Height;
            float thumbLength = ContentSize <= 0.0f
                ? trackLength
                : Math.Clamp(trackLength * ViewportSize / ContentSize, 12.0f, trackLength);
            ThumbSize = Orientation == Orientation.Horizontal
                ? new Float2(thumbLength, Height)
                : new Float2(Width, thumbLength);
            UpdateThumb();
        }
    }
}
