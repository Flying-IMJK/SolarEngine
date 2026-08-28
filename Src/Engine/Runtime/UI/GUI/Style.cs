using System;

namespace SE.GUI
{
    /// <summary>
    /// Describes GUI controls style. Defines default fonts, colors and shared control assets.
    /// </summary>
    public class Style
    {
        private const string DefaultFontPath = "Assets/Fonts/Roboto-Regular";
        private const float TitleFontSize = 18.0f;
        private const float LargeFontSize = 14.0f;
        private const float MediumFontSize = 9.0f;
        private const float SmallFontSize = 9.0f;

        private static Style _current = CreateLightStyle();

        /// <summary>
        /// Global GUI style used by all controls.
        /// </summary>
        public static Style Current
        {
            get => _current;
            set
            {
                _current = value;
                _current?.EnsureDefaultFonts();
            }
        }

        private Font? _fontTitle;
        private Font? _fontLarge;
        private Font? _fontMedium;
        private Font? _fontSmall;

        /// <summary>
        /// The title font.
        /// </summary>
        public Font? FontTitle
        {
            get => _fontTitle;
            set => _fontTitle = value;
        }

        /// <summary>
        /// The large font.
        /// </summary>
        public Font? FontLarge
        {
            get => _fontLarge;
            set => _fontLarge = value;
        }

        /// <summary>
        /// The medium font.
        /// </summary>
        public Font? FontMedium
        {
            get => _fontMedium;
            set => _fontMedium = value;
        }

        /// <summary>
        /// The small font.
        /// </summary>
        public Font? FontSmall
        {
            get => _fontSmall;
            set => _fontSmall = value;
        }

        /// <summary>
        /// The background color.
        /// </summary>
        public Color Background;

        /// <summary>
        /// The light background color.
        /// </summary>
        public Color LightBackground;

        /// <summary>
        /// The drag window color.
        /// </summary>
        public Color DragWindow;

        /// <summary>
        /// The foreground color.
        /// </summary>
        public Color Foreground;

        /// <summary>
        /// The foreground grey.
        /// </summary>
        public Color ForegroundGrey;

        /// <summary>
        /// The foreground disabled color.
        /// </summary>
        public Color ForegroundDisabled;

        /// <summary>
        /// The foreground color in viewports.
        /// </summary>
        public Color ForegroundViewport;

        /// <summary>
        /// The background highlighted color.
        /// </summary>
        public Color BackgroundHighlighted;

        /// <summary>
        /// The border highlighted color.
        /// </summary>
        public Color BorderHighlighted;

        /// <summary>
        /// The background selected color.
        /// </summary>
        public Color BackgroundSelected;

        /// <summary>
        /// The border selected color.
        /// </summary>
        public Color BorderSelected;

        /// <summary>
        /// The background normal color.
        /// </summary>
        public Color BackgroundNormal;

        /// <summary>
        /// The border normal color.
        /// </summary>
        public Color BorderNormal;

        /// <summary>
        /// The text color.
        /// </summary>
        public Color TextColor;

        /// <summary>
        /// The text box background color.
        /// </summary>
        public Color TextBoxBackground;

        /// <summary>
        /// The text box selected background color.
        /// </summary>
        public Color TextBoxBackgroundSelected;

        /// <summary>
        /// The collection background color.
        /// </summary>
        public Color CollectionBackgroundColor;

        /// <summary>
        /// The normal progress color.
        /// </summary>
        public Color ProgressNormal;

        /// <summary>
        /// The selection and drag drop highlight color.
        /// </summary>
        public Color Selection;

        /// <summary>
        /// The selection and drag drop highlight border color.
        /// </summary>
        public Color SelectionBorder;

        /// <summary>
        /// The status bar style.
        /// </summary>
        public StatusbarStyle Statusbar;

        /// <summary>
        /// The arrow right icon.
        /// </summary>
        public SpriteHandle ArrowRight;

        /// <summary>
        /// The arrow down icon.
        /// </summary>
        public SpriteHandle ArrowDown;

        /// <summary>
        /// The search icon.
        /// </summary>
        public SpriteHandle Search;

        /// <summary>
        /// The settings icon.
        /// </summary>
        public SpriteHandle Settings;

        /// <summary>
        /// The cross icon.
        /// </summary>
        public SpriteHandle Cross;

        /// <summary>
        /// The CheckBox intermediate icon.
        /// </summary>
        public SpriteHandle CheckBoxIntermediate;

        /// <summary>
        /// The CheckBox tick icon.
        /// </summary>
        public SpriteHandle CheckBoxTick;

        /// <summary>
        /// The status bar size grip icon.
        /// </summary>
        public SpriteHandle StatusBarSizeGrip;

        /// <summary>
        /// The translate icon.
        /// </summary>
        public SpriteHandle Translate;

        /// <summary>
        /// The rotate icon.
        /// </summary>
        public SpriteHandle Rotate;

        /// <summary>
        /// The scale icon.
        /// </summary>
        public SpriteHandle Scale;

        /// <summary>
        /// The scalar icon.
        /// </summary>
        public SpriteHandle Scalar;

        /// <summary>
        /// The shared tooltip control used by controls if no custom tooltip is provided.
        /// </summary>
        public Tooltip? SharedTooltip;

        /// <summary>
        /// Ensures the style has all default interface fonts assigned.
        /// </summary>
        public void EnsureDefaultFonts()
        {
            FontAsset? defaultFont = AssetContent.LoadAsyncInternal<FontAsset>(DefaultFontPath);
            if (defaultFont == null)
            {
                return;
            }

            defaultFont.WaitForLoaded();

            _fontTitle = defaultFont.CreateFont(TitleFontSize);
            _fontLarge = defaultFont.CreateFont(LargeFontSize);
            _fontMedium = defaultFont.CreateFont(MediumFontSize);
            _fontSmall = defaultFont.CreateFont(SmallFontSize);
        }

        /// <summary>
        /// Creates the default dark style.
        /// </summary>
        public static Style CreateDefaultStyle()
        {
            var style = new Style
            {
                Background = Color.FromRGBA(0x1CFF1C1C),
                LightBackground = Color.FromRGBA(0x2DFF2D30),
                Foreground = Color.FromRGBA(0xFFFFFFFF),
                ForegroundGrey = Color.FromRGBA(0xA9FFA9B3),
                ForegroundDisabled = Color.FromRGBA(0x78FF7883),
                ForegroundViewport = Color.FromRGBA(0xFFFFFFFF),
                BackgroundHighlighted = Color.FromRGBA(0x54FF545C),
                BorderHighlighted = Color.FromRGBA(0x6AFF6A75),
                BackgroundSelected = Color.FromRGBA(0x7AFF00CC),
                BorderSelected = Color.FromRGBA(0x97FF1CEA),
                BackgroundNormal = Color.FromRGBA(0x3FFF3F46),
                BorderNormal = Color.FromRGBA(0x54FF545C),
                TextBoxBackground = Color.FromRGBA(0x33FF3337),
                TextBoxBackgroundSelected = Color.FromRGBA(0x3FFF3F46),
                CollectionBackgroundColor = Color.FromRGBA(0xCC14CCCC),
                ProgressNormal = Color.FromRGBA(0xD3FF0A28),
                Selection = Color.Orange * 0.4f,
                SelectionBorder = Color.Orange,
                Statusbar = new StatusbarStyle
                {
                    PlayMode = Color.FromRGBA(0x91FF2F35),
                    Failed = Color.FromRGBA(0x24FF9C24),
                    Loading = Color.FromRGBA(0x2DFF2D30),
                },
            };

            style.TextColor = style.Foreground;
            style.DragWindow = style.BackgroundSelected * 0.7f;
            style.EnsureDefaultFonts();
            return style;
        }

        /// <summary>
        /// Creates the default light style.
        /// </summary>
        public static Style CreateLightStyle()
        {
            var style = new Style
            {
                Background = new Color(0.92f, 0.92f, 0.92f, 1.0f),
                LightBackground = new Color(0.84f, 0.84f, 0.88f, 1.0f),
                DragWindow = new Color(0.0f, 0.26f, 0.43f, 0.70f),
                Foreground = new Color(1.0f, 1.0f, 1.0f, 1.0f),
                ForegroundGrey = new Color(0.30f, 0.30f, 0.31f, 1.0f),
                ForegroundDisabled = new Color(0.45f, 0.45f, 0.49f, 1.0f),
                ForegroundViewport = new Color(1.0f, 1.0f, 1.0f, 1.0f),
                BackgroundHighlighted = new Color(0.59f, 0.59f, 0.64f, 1.0f),
                BorderHighlighted = new Color(0.50f, 0.50f, 0.55f, 1.0f),
                BackgroundSelected = new Color(0.00f, 0.46f, 0.78f, 0.78f),
                BorderSelected = new Color(0.11f, 0.57f, 0.88f, 0.65f),
                BackgroundNormal = new Color(0.67f, 0.67f, 0.75f, 1.0f),
                BorderNormal = new Color(0.59f, 0.59f, 0.64f, 1.0f),
                TextColor = new Color(0.0f, 0.0f, 0.0f, 1.0f),
                TextBoxBackground = new Color(0.75f, 0.75f, 0.81f, 1.0f),
                TextBoxBackgroundSelected = new Color(0.73f, 0.73f, 0.80f, 1.0f),
                CollectionBackgroundColor = new Color(0.85f, 0.85f, 0.88f, 1.0f),
                ProgressNormal = new Color(0.03f, 0.65f, 0.12f, 1.0f),
                Selection = Color.Orange * 0.4f,
                SelectionBorder = Color.Orange,
            };

            style.EnsureDefaultFonts();
            return style;
        }

        /// <summary>
        /// Style for the status bar.
        /// </summary>
        [Serializable]
        public struct StatusbarStyle
        {
            /// <summary>
            /// Color of the status bar when in Play Mode.
            /// </summary>
            public Color PlayMode;

            /// <summary>
            /// Color of the status bar while loading.
            /// </summary>
            public Color Loading;

            /// <summary>
            /// Color of the status bar in failed state.
            /// </summary>
            public Color Failed;
        }
    }
}
