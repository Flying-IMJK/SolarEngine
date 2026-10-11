namespace SE.GUI
{
    public enum BrushFilter
    {
        Point = 0,
        Linear = 1,
    }

    /// <summary>
    /// Draws an image-like value into a GUI rectangle.
    /// </summary>
    public interface IBrush
    {
        Float2 Size
        {
            get;
        }
        /// <summary>
        /// Draws the brush using root logical coordinates.
        /// </summary>
        void Draw(Rectangle bounds, Color color);
    }
}
