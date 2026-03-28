#pragma once
#include "Runtime/Resource/AssetRef.h"
#include "Runtime/UI/GUI/ContainerControl.h"

namespace SE
{
    class SpriteAtlas;
    class Texture;
    class MaterialInstance;
}

namespace SE::Editor
{
    class ContextMenuButton;
    class ContextMenu;
    /// <summary>
    /// Base class for texture previews. Draws a surface in the UI and supports view moving/zooming.
    /// </summary>
    class TexturePreviewBase : public ContainerControl
    {
    private:
        Rectangle m_TextureRect;
        Float2 m_LastMousePos, m_ViewPos;
        float m_ViewScale = 1.0f;
        bool m_IsMouseDown;

    public:
        /// <summary>
        /// Moves the view to the center.
        /// </summary>
        void CenterView();

        /// <inheritdoc />
        void Draw() override;

        /// <inheritdoc />
        void OnMouseEnter(Float2 location) override;

        /// <inheritdoc />
        void OnMouseMove(Float2 location) override;

        /// <inheritdoc />
        void OnMouseLeave() override;

        /// <inheritdoc />
        bool OnMouseWheel(Float2 location, float delta) override;

        /// <inheritdoc />
        bool OnMouseDown(Float2 location, MouseButton button) override;

        /// <inheritdoc />
        bool OnMouseUp(Float2 location, MouseButton button) override;

    protected:
        /// <inheritdoc />
        TexturePreviewBase();


        /// <summary>
        /// Updates the texture rectangle.
        /// </summary>
        void UpdateTextureRect();

        /// <summary>
        /// Calculates the texture rectangle.
        /// </summary>
        /// <param name="rect">The rectangle.</param>
        virtual void CalculateTextureRect(Rectangle& rect) = 0;

        /// <summary>
        /// Calculates the texture rect fr the given texture and the view size.
        /// </summary>
        /// <param name="textureSize">Size of the texture.</param>
        /// <param name="viewSize">Size of the view.</param>
        /// <param name="result">The result.</param>
        static void CalculateTextureRect(Float2 textureSize, Float2 viewSize, Rectangle& result);

        /// <summary>
        /// Draws the texture.
        /// </summary>
        /// <param name="rect">The target texture view rectangle.</param>
        virtual void DrawTexture(Rectangle& rect) = 0;

        /// <summary>
        /// Gets the texture view rect (scaled and offseted).
        /// </summary>
        Rectangle GetTextureViewRect();

        /// <inheritdoc />
        void OnSizeChanged() override;
    };

    /// <summary>
    /// Texture channel flags.
    /// </summary>
    enum ChannelFlags
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
        All = Red | Green | Blue | Alpha
    };

    /*/// <summary>
    /// Base class for texture previews with custom drawing features. Uses in-build postFx material to render a texture.
    /// </summary>
    /// <seealso cref="TexturePreviewBase" />
    class TexturePreviewCustomBase : TexturePreviewBase
    {
    private:
        ChannelFlags _channelFlags = ChannelFlags::All;
        bool _usePointSampler = false;
        float _mipLevel = -1;
        ContextMenu* _mipWidgetMenu;
        ContextMenuButton* _filterWidgetPointButton;
        ContextMenuButton* _filterWidgetLinearButton;

    public:
        /// <summary>
        /// Gets or sets the view channels to show.
        /// </summary>
        ChannelFlags ViewChannels
        {
            get => _channelFlags;
            set
            {
                if (_channelFlags != value)
                {
                    _channelFlags = value;
                    UpdateMask();
                }
            }
        }

        /// <summary>
        /// Gets or sets a value indicating whether use point sampler when drawing the texture. The default value is false.
        /// </summary>
        bool UsePointSampler
        {
            get => _usePointSampler;
            set
            {
                if (_usePointSampler != value)
                {
                    _usePointSampler = value;
                    _previewMaterial.SetParameterValue("PointSampler", value);
                }
            }
        }

        /// <summary>
        /// Gets or sets the mip level to show. The default value is -1.
        /// </summary>
        float MipLevel
        {
            get => _mipLevel;
            set
            {
                if (!Mathf.NearEqual(_mipLevel, value))
                {
                    _mipLevel = value;
                    _previewMaterial.SetParameterValue("Mip", value);
                }
            }
        }

        /// <inheritdoc />
        void OnDestroy() override;

    protected:
        /// <summary>
        /// The preview material instance used to draw texture.
        /// </summary>
        MaterialInstance* _previewMaterial;

        /// <inheritdoc />
        /// <param name="useWidgets">True if show viewport widgets.</param>
        TexturePreviewCustomBase(bool useWidgets)
        {
            // Create preview material (virtual)
            var baseMaterial = FlaxEngine.Content.LoadAsyncInternal<Material>("Editor/TexturePreviewMaterial");
            if (baseMaterial == null)
                throw new Exception("Cannot load texture preview material.");
            _previewMaterial = baseMaterial.CreateVirtualInstance();
            if (_previewMaterial == null)
                throw new Exception("Failed to create virtual material instance for preview material.");

            // Add widgets
            if (useWidgets)
            {
                // Channels widget
                var channelsWidget = new ViewportWidgetsContainer(ViewportWidgetLocation.UpperLeft);
                //
                var channelR = new ViewportWidgetButton("R", SpriteHandle.Invalid, null, true)
                {
                    Checked = true,
                    TooltipText = "Show/hide texture red channel",
                    Parent = channelsWidget
                };
                channelR.Toggled += button => ViewChannels = button.Checked ? ViewChannels | ChannelFlags.Red : (ViewChannels & ~ChannelFlags.Red);
                var channelG = new ViewportWidgetButton("G", SpriteHandle.Invalid, null, true)
                {
                    Checked = true,
                    TooltipText = "Show/hide texture green channel",
                    Parent = channelsWidget
                };
                channelG.Toggled += button => ViewChannels = button.Checked ? ViewChannels | ChannelFlags.Green : (ViewChannels & ~ChannelFlags.Green);
                var channelB = new ViewportWidgetButton("B", SpriteHandle.Invalid, null, true)
                {
                    Checked = true,
                    TooltipText = "Show/hide texture blue channel",
                    Parent = channelsWidget
                };
                channelB.Toggled += button => ViewChannels = button.Checked ? ViewChannels | ChannelFlags.Blue : (ViewChannels & ~ChannelFlags.Blue);
                var channelA = new ViewportWidgetButton("A", SpriteHandle.Invalid, null, true)
                {
                    Checked = true,
                    TooltipText = "Show/hide texture alpha channel",
                    Parent = channelsWidget
                };
                channelA.Toggled += button => ViewChannels = button.Checked ? ViewChannels | ChannelFlags.Alpha : (ViewChannels & ~ChannelFlags.Alpha);
                //
                channelsWidget.Parent = this;

                // Mip widget
                var mipWidget = new ViewportWidgetsContainer(ViewportWidgetLocation.UpperLeft);
                _mipWidgetMenu = new ContextMenu();
                _mipWidgetMenu.VisibleChanged += OnMipWidgetMenuOnVisibleChanged;
                var mipWidgetButton = new ViewportWidgetButton("Mip", SpriteHandle.Invalid, _mipWidgetMenu)
                {
                    TooltipText = "The mip level to show. The default is -1.",
                    Parent = mipWidget
                };
                //
                mipWidget.Parent = this;

                // Filter widget
                var filterWidget = new ViewportWidgetsContainer(ViewportWidgetLocation.UpperLeft);
                var filterWidgetMenu = new ContextMenu();
                filterWidgetMenu.VisibleChanged += OnFilterWidgetMenuVisibleChanged;
                _filterWidgetPointButton = filterWidgetMenu.AddButton("Point", () => UsePointSampler = true);
                _filterWidgetLinearButton = filterWidgetMenu.AddButton("Linear", () => UsePointSampler = false);
                var filterWidgetButton = new ViewportWidgetButton("Filter", SpriteHandle.Invalid, filterWidgetMenu)
                {
                    TooltipText = "The texture preview filtering mode. The default is Linear.",
                    Parent = filterWidget
                };
                //
                filterWidget.Parent = this;
            }

            // Wait for base (don't want to async material parameters set due to async loading)
            baseMaterial.WaitForLoaded();
        }

        /// <summary>
        /// Sets the texture to draw (material parameter).
        /// </summary>
        /// <param name="value">The value.</param>
        void SetTexture(object value)
        {
            _previewMaterial.SetParameterValue("Texture", value);
            UpdateTextureRect();
        }

        /// <inheritdoc />
        void PerformLayoutBeforeChildren() override
        {
            base.PerformLayoutBeforeChildren();

            ViewportWidgetsContainer.ArrangeWidgets(this);
        }

    private:
        void OnFilterWidgetMenuVisibleChanged(Control control)
        {
            if (!control.Visible)
                return;

            _filterWidgetPointButton.Checked = UsePointSampler;
            _filterWidgetLinearButton.Checked = !UsePointSampler;
        }

        void OnMipWidgetMenuOnVisibleChanged(Control control)
        {
            if (!control.Visible)
                return;

            var textureObj = _previewMaterial.GetParameterValue("Texture");

            if (textureObj is TextureBase texture && !texture.WaitForLoaded())
            {
                _mipWidgetMenu.ItemsContainer.DisposeChildren();
                var mipLevels = texture.MipLevels;
                for (int i = -1; i < mipLevels; i++)
                {
                    var button = _mipWidgetMenu.AddButton(i.ToString(), OnMipWidgetClicked);
                    button.Tag = i;
                    if (i == -1)
                        button.TooltipText = "Default mip.";
                    button.Checked = Mathf.Abs(MipLevel - i) < 0.9f;
                }
            }
        }

        void OnMipWidgetClicked(ContextMenuButton button)
        {
            MipLevel = (int)button.Tag;
        }

        void UpdateMask()
        {
            Vector4 mask = Vector4.One;
            if ((_channelFlags & ChannelFlags.Red) == 0)
                mask.X = 0;
            if ((_channelFlags & ChannelFlags.Green) == 0)
                mask.Y = 0;
            if ((_channelFlags & ChannelFlags.Blue) == 0)
                mask.Z = 0;
            if ((_channelFlags & ChannelFlags.Alpha) == 0)
                mask.W = 0;
            _previewMaterial.SetParameterValue("Mask", mask);
        }
    };*/

    /// <summary>
    /// Texture preview GUI control. Draws <see cref="FlaxEngine.Texture"/> in the UI and supports view moving/zomming.
    /// </summary>
    /// <seealso cref="TexturePreviewBase" />
    class SimpleTexturePreview : public TexturePreviewBase
    {
    private:
        AssetRef<Texture> m_Asset;

    public:
        /// <summary>
        /// Gets or sets the asset to preview.
        /// </summary>
        PRO(Asset, SimpleTexturePreview, Texture*, __GetAsset, __SetAsset);

        /// <summary>
        /// Gets or sets the color used to multiply texture colors.
        /// </summary>
        Color Color = Colors::White;

        /// <inheritdoc />
        void CalculateTextureRect(Rectangle& rect) override;

        /// <inheritdoc />
        void DrawTexture(Rectangle& rect) override;

    private:
        Texture* __GetAsset();
        void __SetAsset(Texture* value);
    };

    /// <summary>
    /// Sprite atlas preview GUI control. Draws <see cref="SpriteAtlas"/> in the UI and supports view moving/zomming.
    /// </summary>
    /// <seealso cref="TexturePreviewBase" />
    class SimpleSpriteAtlasPreview : public TexturePreviewBase
    {
    private:
        AssetRef<SpriteAtlas> _asset;

    public:
        /// <summary>
        /// Gets or sets the asset to preview.
        /// </summary>
        PRO(Asset, SimpleSpriteAtlasPreview, SpriteAtlas*, __GetAsset, __SetAsset);

        /// <summary>
        /// Gets or sets the color used to multiply texture colors.
        /// </summary>
        Color Color = Colors::White;

    protected:
        /// <inheritdoc />
        void CalculateTextureRect(Rectangle& rect) override;

        /// <inheritdoc />
        void DrawTexture(Rectangle& rect) override;

    private:
        SpriteAtlas* __GetAsset();
        void __SetAsset(SpriteAtlas* value);
    };

    /*/// <summary>
    /// Texture preview GUI control. Draws <see cref="FlaxEngine.Texture"/> in the UI and supports view moving/zooming.
    /// Supports texture channels masking and color transformations.
    /// </summary>
    /// <seealso cref="TexturePreviewCustomBase" />
    public class TexturePreview : TexturePreviewCustomBase
    {
        private TextureBase _asset;

        /// <summary>
        /// Gets or sets the texture to preview.
        /// </summary>
        public TextureBase Asset
        {
            get => _asset;
            set
            {
                if (_asset != value)
                {
                    _asset = value;
                    SetTexture(_asset);
                }
            }
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="TexturePreview"/> class.
        /// </summary>
        /// <param name="useWidgets">True if show viewport widgets.</param>
        /// <inheritdoc />
        public TexturePreview(bool useWidgets)
        : base(useWidgets)
        {
        }

        /// <inheritdoc />
        protected override void CalculateTextureRect(out Rectangle rect)
        {
            CalculateTextureRect(_asset != null ? _asset.Size : new Float2(100), Size, out rect);
        }

        /// <inheritdoc />
        protected override void DrawTexture(ref Rectangle rect)
        {
            // Background
            Render2D.FillRectangle(rect, Color.Gray);

            // Check if has loaded asset
            if (_asset && _asset.IsLoaded)
            {
                Render2D.DrawMaterial(_previewMaterial, rect);
            }
        }
    }

    /// <summary>
    /// Sprite atlas preview GUI control. Draws <see cref="FlaxEngine.SpriteAtlas"/> in the UI and supports view moving/zomming.
    /// Supports texture channels masking and color transformations.
    /// </summary>
    /// <seealso cref="TexturePreviewCustomBase" />
    public class SpriteAtlasPreview : TexturePreviewCustomBase
    {
        private SpriteAtlas _asset;

        /// <summary>
        /// Gets or sets the sprite atlas to preview.
        /// </summary>
        public SpriteAtlas Asset
        {
            get => _asset;
            set
            {
                if (_asset != value)
                {
                    _asset = value;
                    SetTexture(_asset);
                }
            }
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="SpriteAtlasPreview"/> class.
        /// </summary>
        /// <param name="useWidgets">True if show viewport widgets.</param>
        /// <inheritdoc />
        public SpriteAtlasPreview(bool useWidgets)
        : base(useWidgets)
        {
        }

        /// <inheritdoc />
        protected override void CalculateTextureRect(out Rectangle rect)
        {
            CalculateTextureRect(_asset != null ? _asset.Size : new Float2(100), Size, out rect);
        }

        /// <inheritdoc />
        protected override void DrawTexture(ref Rectangle rect)
        {
            // Background
            Render2D.FillRectangle(rect, Color.Gray);

            // Check if has loaded asset
            if (_asset && _asset.IsLoaded)
            {
                Render2D.DrawMaterial(_previewMaterial, rect);
            }
        }
    }*/

} // SE

