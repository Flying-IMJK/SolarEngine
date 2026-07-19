namespace SE.GUI
{
    /// <summary>
    /// A container that renders a border around its bounds.
    /// </summary>
    public class Border : ContainerControl
    {
        public Border()
        {
            AutoFocus = false;
            BorderColor = Style.Current.Foreground;
            BorderWidth = 1.0f;
        }

        public Border(Rectangle bounds)
            : this()
        {
            Bounds = bounds;
        }

        public Color BorderColor { get; set; }
        public float BorderWidth { get; set; }

        protected override void OnDraw()
        {
            base.OnDraw();
            if (BorderWidth <= 0.0f || BorderColor.A <= 0.0f)
                return;

            Rectangle bounds = ScreenBounds;
            Color color = BorderColor;
            Render2D.DrawRectangle(ref bounds, ref color, BorderWidth);
        }
    }
}
