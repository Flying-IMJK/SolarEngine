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
        private GuiRect _textureRect;
        private GuiPoint _lastMousePos;
        private GuiPoint _viewPos;
        private float _viewScale = 1.0f;
        private bool _isMouseDown;

        protected TexturePreviewBase()
            : base(new Rectangle(0, 0, 256, 256))
        {
            SetBounds(0, 0, 256, 256);
        }

        public GuiRect TextureRect => _textureRect;
        public GuiPoint ViewPosition => _viewPos;
        public float ViewScale => _viewScale;
        public bool IsMouseDown => _isMouseDown;

        public void CenterView()
        {
            _viewPos = GuiPoint.Zero;
            _viewScale = 1.0f;
            UpdateTextureRect();
        }

        public void BeginMove(GuiPoint location)
        {
            _lastMousePos = location;
            _isMouseDown = true;
        }

        public void Move(GuiPoint location)
        {
            if (!_isMouseDown)
                return;

            _viewPos = new GuiPoint(_viewPos.X + location.X - _lastMousePos.X, _viewPos.Y + location.Y - _lastMousePos.Y);
            _lastMousePos = location;
            UpdateTextureRect();
        }

        public void EndMove()
        {
            _isMouseDown = false;
        }

        public void Zoom(float delta)
        {
            _viewScale = Math.Clamp(_viewScale + delta * 0.1f, 0.05f, 32.0f);
            UpdateTextureRect();
        }

        protected void UpdateTextureRect()
        {
            _textureRect = CalculateTextureRect();
        }

        protected abstract GuiRect CalculateTextureRect();

        protected static GuiRect CalculateTextureRect(GuiSize textureSize, GuiSize viewSize)
        {
            if (textureSize.Width <= 0 || textureSize.Height <= 0 || viewSize.Width <= 0 || viewSize.Height <= 0)
                return new GuiRect(0, 0, 0, 0);

            float scale = Math.Min(viewSize.Width / textureSize.Width, viewSize.Height / textureSize.Height);
            float width = textureSize.Width * scale;
            float height = textureSize.Height * scale;
            return new GuiRect((viewSize.Width - width) * 0.5f, (viewSize.Height - height) * 0.5f, width, height);
        }

        protected GuiRect GetTextureViewRect()
        {
            GuiRect rect = _textureRect;
            return new GuiRect(
                rect.X + _viewPos.X,
                rect.Y + _viewPos.Y,
                rect.Width * _viewScale,
                rect.Height * _viewScale);
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
        public GuiSize TextureSize { get; set; } = new GuiSize(100, 100);

        protected override GuiRect CalculateTextureRect()
        {
            return CalculateTextureRect(TextureSize, new GuiSize(Width, Height));
        }
    }

    public sealed class SimpleSpriteAtlasPreview : TexturePreviewBase
    {
        public object? Asset { get; set; }
        public GuiSize AtlasSize { get; set; } = new GuiSize(100, 100);

        protected override GuiRect CalculateTextureRect()
        {
            return CalculateTextureRect(AtlasSize, new GuiSize(Width, Height));
        }
    }
}
