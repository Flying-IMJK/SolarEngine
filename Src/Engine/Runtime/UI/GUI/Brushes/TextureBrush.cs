using System;

namespace SE.GUI
{
    /// <summary>
    /// A brush backed by a generated managed texture asset API.
    /// </summary>
    public sealed class TextureBrush : IBrush
    {
        public TextureBrush(TextureBase texture)
        {
            Texture = texture ?? throw new ArgumentNullException(nameof(texture));
        }

        public TextureBase Texture { get; }

        public void Draw(Rectangle bounds, Color color)
        {
            Render2D.DrawTexture(Texture, ref bounds, ref color);
        }
    }
}
