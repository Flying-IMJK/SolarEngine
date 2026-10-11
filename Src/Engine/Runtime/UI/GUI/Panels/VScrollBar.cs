namespace SE.GUI
{
    public class VScrollBar : ScrollBar
    {
        public VScrollBar()
        {
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="VScrollBar"/> class.
        /// </summary>
        /// <param name="parent">The parent control.</param>
        /// <param name="x">The x position.</param>
        /// <param name="height">The height.</param>
        /// <param name="width">The width.</param>
        public VScrollBar(ContainerControl parent, float x, float height, float width = 14.0f)
            : base(Orientation.Vertical)
        {
            AnchorPreset = AnchorPresets.VerticalStretchRight;
            Parent = parent;
            Bounds = new Rectangle(x, 0.0f, width, height);
        }

        /// <inheritdoc />
        protected override float TrackSize()
        {
            return Height;
        }
    }
}
