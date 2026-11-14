namespace SE.Editor.GUI
{
    /// <summary>
    /// Draws a cube texture as a reflection sphere using the existing texture preview layout.
    /// </summary>
    public class CubeTexturePreview : TexturePreviewBase
    {
        private CubeTexture m_Asset;

        public CubeTexture Asset
        {
            get
            {
                return m_Asset;
            }
            set
            {
                if (m_Asset != value)
                {
                    m_Asset = value;
                    UpdateTextureRect();
                }
            }
        }

        public Color Color = SE.Color.White;

        protected override void CalculateTextureRect(ref Rectangle rect)
        {
            CalculateTextureRect(Float2.One, Size, ref rect);
        }

        protected override void DrawTexture(ref Rectangle rect)
        {
            if (m_Asset != null && m_Asset.IsLoaded())
            {
                Render2D.DrawCubeTexture(m_Asset, rect, Color);
            }
        }
    }
}
