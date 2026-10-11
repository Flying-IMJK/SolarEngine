namespace SE.GUI
{
    public class HScrollBar : ScrollBar
    {
        public HScrollBar()
        {
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="HScrollBar"/> class.
        /// </summary>
        /// <param name="parent">The parent control.</param>
        /// <param name="y">The y position.</param>
        /// <param name="width">The width.</param>
        /// <param name="height">The height.</param>
        public HScrollBar(ContainerControl parent, float y, float width, float height = 14.0f)
            : base(Orientation.Horizontal)
        {
            AnchorPreset = AnchorPresets.HorizontalStretchBottom;
            Parent = parent;
            Bounds = new Rectangle(0.0f, y, width, height);
        }

        /// <inheritdoc />
        protected override float TrackSize()
        {
            return Width;
        }
    }
}
