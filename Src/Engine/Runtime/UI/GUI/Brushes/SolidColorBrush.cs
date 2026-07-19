namespace SE.GUI
{
    /// <summary>
    /// A brush that fills its destination with a single color.
    /// </summary>
    public sealed class SolidColorBrush : IBrush
    {
        public SolidColorBrush()
            : this(Color.White)
        {
        }

        public SolidColorBrush(Color color)
        {
            Color = color;
        }

        public Color Color { get; set; }

        public void Draw(Rectangle bounds, Color color)
        {
            Color tint = Color * color;
            Render2D.FillRectangle(ref bounds, ref tint);
        }
    }
}
