namespace SE.GUI
{
    /// <summary>
    /// Draws an image-like value into a GUI rectangle.
    /// </summary>
    public interface IBrush
    {
        /// <summary>
        /// Draws the brush using root logical coordinates.
        /// </summary>
        void Draw(Rectangle bounds, Color color);
    }
}
