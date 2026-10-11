namespace SE.GUI
{
    /// <summary>
    /// Draws a sprite using its atlas area.
    /// </summary>
    public sealed class SpriteBrush : IBrush
    {
        public SpriteBrush(SpriteHandle sprite) => Sprite = sprite;
        public SpriteHandle Sprite { get; set; }
        public BrushFilter Filter { get; set; } = BrushFilter.Linear;
        public Float2 Size => Sprite.IsValid ? Sprite.Size : Float2.Zero;

        public void Draw(Rectangle bounds, Color color)
        {
            if (Filter == BrushFilter.Point)
            {
                Render2D.DrawSpritePoint(Sprite, bounds, color);
            }
            else
            {
                Render2D.DrawSprite(Sprite, bounds, color);
            }
        }
    }
}
