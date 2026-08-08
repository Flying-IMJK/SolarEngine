using System;
using SE.GUI;
namespace SE.Editor.GUI
{
    [Flags]
    public enum ChannelFlags
    {
        None = 0,
        Red = 1,
        Green = 2,
        Blue = 4,
        Alpha = 8,
        All = Red | Green | Blue | Alpha,
    }

    public abstract class TexturePreviewBase : ContainerControl
    {
        private Rectangle m_TextureRect;
        private Float2 m_LastMousePos;
        private Float2 m_ViewPos;
        private float m_ViewScale = 1.0f;
        private bool m_IsMouseDown;

        protected TexturePreviewBase()
            : base(new Rectangle(0, 0, 256, 256))
        {
            SetBounds(0, 0, 256, 256);
        }

        public Rectangle TextureRect => m_TextureRect;
        public Float2 ViewPosition => m_ViewPos;
        public float ViewScale => m_ViewScale;
        public bool IsMouseDown => m_IsMouseDown;

        public void CenterView()
        {
            m_ViewPos = Float2.Zero;
            m_ViewScale = 1.0f;
            UpdateTextureRect();
        }

        public void BeginMove(Float2 location)
        {
            m_LastMousePos = location;
            m_IsMouseDown = true;
        }

        public void Move(Float2 location)
        {
            if (!m_IsMouseDown)
                return;

            m_ViewPos = new Float2(m_ViewPos.X + location.X - m_LastMousePos.X, m_ViewPos.Y + location.Y - m_LastMousePos.Y);
            m_LastMousePos = location;
            UpdateTextureRect();
        }

        public void EndMove()
        {
            m_IsMouseDown = false;
        }

        public void Zoom(float delta)
        {
            m_ViewScale = Math.Clamp(m_ViewScale + delta * 0.1f, 0.05f, 32.0f);
            UpdateTextureRect();
        }

        protected void UpdateTextureRect()
        {
            m_TextureRect = CalculateTextureRect();
        }

        protected abstract Rectangle CalculateTextureRect();

        protected static Rectangle CalculateTextureRect(Float2 textureSize, Float2 viewSize)
        {
            Float2 size = Float2.Max(textureSize, Float2.One);
            float aspectRatio = size.X / size.Y;
            float h = viewSize.X / aspectRatio;
            float w = viewSize.Y * aspectRatio;
            if (w > h)
            {
                float diff = (viewSize.Y - h) * 0.5f;
                return new Rectangle(0, diff, viewSize.X, h);
            }
            else
            {
                float diff = (viewSize.X - w) * 0.5f;
                return new Rectangle(diff, 0, w, viewSize.Y);
            }
        }

        protected Rectangle GetTextureViewRect()
        {
            Rectangle rect = m_TextureRect;
            return new Rectangle(
                rect.X + m_ViewPos.X,
                rect.Y + m_ViewPos.Y,
                rect.Width * m_ViewScale,
                rect.Height * m_ViewScale);
        }

        protected override void OnBoundsChanged(bool locationChanged, bool sizeChanged)
        {
            base.OnBoundsChanged(locationChanged, sizeChanged);
            UpdateTextureRect();
        }
    }

    public sealed class SimpleTexturePreview : TexturePreviewBase
    {
        public object? Asset { get; set; }
        public Float2 TextureSize { get; set; } = new Float2(100, 100);

        protected override Rectangle CalculateTextureRect()
        {
            return CalculateTextureRect(TextureSize, new Float2(Width, Height));
        }
    }

    public sealed class SimpleSpriteAtlasPreview : TexturePreviewBase
    {
        public object? Asset { get; set; }
        public Float2 AtlasSize { get; set; } = new Float2(100, 100);

        protected override Rectangle CalculateTextureRect()
        {
            return CalculateTextureRect(AtlasSize, new Float2(Width, Height));
        }
    }
}
