using SE.GUI;

namespace SE.Editor
{
    /// <summary>
    /// Thumbnails GUI root control.
    /// </summary>
    public class PreviewRoot : ContainerControl
    {
        /// <summary>
        /// The item accent color to draw.
        /// </summary>
        public Color AccentColor;

        /// <inheritdoc />
        public PreviewRoot() : base(0, 0, 64, 64)
        {
            AutoFocus = false;
            AccentColor = Color.Pink;
            IsLayoutLocked = false;
        }

        /// <inheritdoc />
        public override void Draw()
        {
            base.Draw();

            // Draw accent
            const float accentHeight = 2;
            Render2D.FillRectangle(new Rectangle(0, Height - accentHeight, Width, accentHeight),AccentColor);
        }
    }
}
