#nullable disable annotations

using SE.GUI;

namespace SE.Editor.GUI
{
    /// <summary>
    /// Texture channel flags.
    /// </summary>
    public enum ChannelFlags
    {
        /// <summary>
        /// The none.
        /// </summary>
        None = 0,

        /// <summary>
        /// The red channel.
        /// </summary>
        Red = 1,

        /// <summary>
        /// The green channel.
        /// </summary>
        Green = 2,

        /// <summary>
        /// The blue channel.
        /// </summary>
        Blue = 4,

        /// <summary>
        /// The alpha channel.
        /// </summary>
        Alpha = 8,

        /// <summary>
        /// All texture channels.
        /// </summary>
        All = Red | Green | Blue | Alpha,
    }

    /// <summary>
    /// Base class for texture previews. Draws a surface in the UI and supports view moving/zooming.
    /// </summary>
    public abstract class TexturePreviewBase : ContainerControl
    {
        private Rectangle m_TextureRect;
        private Float2 m_LastMousePos;
        private Float2 m_ViewPos;
        private float m_ViewScale = 1.0f;
        private bool m_IsMouseDown;

        /// <summary>
        /// Moves the view to the center.
        /// </summary>
        public void CenterView()
        {
            m_ViewScale = 1.0f;
            m_ViewPos = Float2.Zero;
        }

        /// <inheritdoc />
        public override void Draw()
        {
            Render2D.PushClip(new Rectangle(Float2.Zero, Size));

            // Calculate texture view rectangle
            UpdateTextureRect();
            Rectangle textureRect = GetTextureViewRect();

            // Call drawing
            DrawTexture(ref textureRect);

            // Add overlay during debugger breakpoint hang
            /*if (Editor.Instance.Simulation.IsDuringBreakpointHang)
            {
                Rectangle bounds = new Rectangle(Float2.Zero, Size);
                Render2D.FillRectangle(bounds, new Color(0.0f, 0.0f, 0.0f, 0.2f));
                Render2D.RenderText(Style.Current.FontLarge, "Debugger breakpoint hit...", bounds, Color.White, TextAlignment.Center, TextAlignment.Center);
            }*/

            Render2D.PopClip();

            base.Draw();
        }

        /// <inheritdoc />
        public override void OnMouseEnter(Float2 location)
        {
            // Store mouse position
            m_LastMousePos = location;

            base.OnMouseEnter(location);
        }

        /// <inheritdoc />
        public override void OnMouseMove(Float2 location)
        {
            // Check if mouse is down
            if (m_IsMouseDown)
            {
                // Calculate mouse delta
                Float2 delta = location - m_LastMousePos;

                // Move view
                m_ViewPos += delta;
            }

            // Store mouse position
            m_LastMousePos = location;

            base.OnMouseMove(location);
        }

        /// <inheritdoc />
        public override void OnMouseLeave()
        {
            // Clear flag
            m_IsMouseDown = false;
            Cursor = CursorType.Default;

            base.OnMouseLeave();
        }

        /// <inheritdoc />
        public override bool OnMouseWheel(Float2 location, float delta)
        {
            if (base.OnMouseWheel(location, delta))
            {
                return true;
            }

            // Zoom
            float prevScale = m_ViewScale;
            m_ViewScale = Mathf.Clamp(m_ViewScale + delta * 0.24f, 0.001f, 20.0f);

            // Compensate for the Rectangle.MakeScaled
            Float2 sizeDelta = (m_ViewScale - prevScale) * m_TextureRect.Size * 0.5f;
            m_ViewPos += sizeDelta * 0.5f;

            // Move to zoom position
            Float2 locationOnTexture = (location - m_TextureRect.Location) / m_TextureRect.Size;
            m_ViewPos -= sizeDelta * locationOnTexture;

            return true;
        }

        /// <inheritdoc />
        public override bool OnMouseDown(Float2 location, MouseButton button)
        {
            if (base.OnMouseDown(location, button))
            {
                return true;
            }

            // Set flag
            m_IsMouseDown = true;
            m_LastMousePos = location;
            Cursor = CursorType.SizeAll;

            return true;
        }

        /// <inheritdoc />
        public override bool OnMouseUp(Float2 location, MouseButton button)
        {
            if (base.OnMouseUp(location, button))
            {
                return true;
            }

            // Clear flag
            m_IsMouseDown = false;
            Cursor = CursorType.Default;

            return true;
        }

        /// <inheritdoc />
        protected TexturePreviewBase()
        {
            AnchorPreset = AnchorPresets.StretchAll;
            Offsets = Margin.Zero;
        }

        /// <summary>
        /// Updates the texture rectangle.
        /// </summary>
        protected void UpdateTextureRect()
        {
            CalculateTextureRect(ref m_TextureRect);
        }

        /// <summary>
        /// Calculates the texture rectangle.
        /// </summary>
        /// <param name="rect">The rectangle.</param>
        protected abstract void CalculateTextureRect(ref Rectangle rect);

        /// <summary>
        /// Calculates the texture rect fr the given texture and the view size.
        /// </summary>
        /// <param name="textureSize">Size of the texture.</param>
        /// <param name="viewSize">Size of the view.</param>
        /// <param name="result">The result.</param>
        protected static void CalculateTextureRect(Float2 textureSize, Float2 viewSize, ref Rectangle result)
        {
            Float2 size = Float2.Max(textureSize, Float2.One);
            float aspectRatio = size.X / size.Y;
            float h = viewSize.X / aspectRatio;
            float w = viewSize.Y * aspectRatio;
            if (w > h)
            {
                float diff = (viewSize.Y - h) * 0.5f;
                result = new Rectangle(0, diff, viewSize.X, h);
            }
            else
            {
                float diff = (viewSize.X - w) * 0.5f;
                result = new Rectangle(diff, 0, w, viewSize.Y);
            }
        }

        /// <summary>
        /// Draws the texture.
        /// </summary>
        /// <param name="rect">The target texture view rectangle.</param>
        protected abstract void DrawTexture(ref Rectangle rect);

        /// <summary>
        /// Gets the texture view rect (scaled and offseted).
        /// </summary>
        protected Rectangle GetTextureViewRect()
        {
            return (m_TextureRect + m_ViewPos) * m_ViewScale;
        }

        /// <inheritdoc />
        protected override void OnSizeChanged()
        {
            base.OnSizeChanged();

            // Update texture rectangle and move view to the center
            UpdateTextureRect();
            CenterView();
        }
    }

    /// <summary>
    /// Texture preview GUI control. Draws <see cref="Texture"/> in the UI and supports view moving/zomming.
    /// </summary>
    /// <seealso cref="TexturePreviewBase" />
    public class SimpleTexturePreview : TexturePreviewBase
    {
        private Texture m_Asset;

        /// <summary>
        /// Gets or sets the asset to preview.
        /// </summary>
        public Texture Asset
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

        /// <summary>
        /// Gets or sets the color used to multiply texture colors.
        /// </summary>
        public Color Color = SE.Color.White;

        /// <inheritdoc />
        protected override void CalculateTextureRect(ref Rectangle rect)
        {
            CalculateTextureRect(m_Asset != null ? m_Asset.Size() : new Float2(100), Size, ref rect);
        }

        /// <inheritdoc />
        protected override void DrawTexture(ref Rectangle rect)
        {
            // Background
            Render2D.FillRectangle(rect, SE.Color.Gray);

            // Check if has loaded asset
            if (m_Asset != null && m_Asset.IsLoaded())
            {
                Render2D.DrawTexture(m_Asset, rect, Color);
            }
        }
    }

    /// <summary>
    /// Sprite atlas preview GUI control. Draws <see cref="SpriteAtlas"/> in the UI and supports view moving/zomming.
    /// </summary>
    /// <seealso cref="TexturePreviewBase" />
    public class SimpleSpriteAtlasPreview : TexturePreviewBase
    {
        private SpriteAtlas m_Asset;

        /// <summary>
        /// Gets or sets the asset to preview.
        /// </summary>
        public SpriteAtlas Asset
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

        /// <summary>
        /// Gets or sets the color used to multiply texture colors.
        /// </summary>
        public Color Color = SE.Color.White;

        /// <inheritdoc />
        protected override void CalculateTextureRect(ref Rectangle rect)
        {
            CalculateTextureRect(m_Asset != null ? m_Asset.Size() : new Float2(100), Size, ref rect);
        }

        /// <inheritdoc />
        protected override void DrawTexture(ref Rectangle rect)
        {
            // Background
            Render2D.FillRectangle(rect, SE.Color.Gray);

            // Check if has loaded asset
            if (m_Asset != null && m_Asset.IsLoaded())
            {
                Render2D.DrawTexture(m_Asset, rect, Color);
            }
        }
    }
}
