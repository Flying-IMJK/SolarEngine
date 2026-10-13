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

/*            _fontTitle = defaultFont.CreateFont(TitleFontSize);
            _fontLarge = defaultFont.CreateFont(LargeFontSize);
            _fontMedium = defaultFont.CreateFont(MediumFontSize);
            _fontSmall = defaultFont.CreateFont(SmallFontSize);*/
        }

        /// <summary>
        /// Creates the default dark style.
        /// </summary>
        public static Style CreateDefaultStyle()
        {
            var style = new Style
            {
                Background = new Color(32, 34, 37),
                LightBackground = new Color(41, 44, 48),
                Foreground = new Color(229, 232, 235),
                ForegroundGrey = new Color(181, 187, 195),
                ForegroundDisabled = new Color(130, 139, 149),
                ForegroundViewport = new Color(255, 255, 255),
                BackgroundHighlighted = new Color(52, 57, 64),
                BorderHighlighted = new Color(74, 89, 102),
                BackgroundSelected = new Color(53, 74, 90),
                BorderSelected = new Color(100, 166, 203),
                BackgroundNormal = new Color(45, 50, 56),
                BorderNormal = new Color(69, 75, 82),
                TextColor = new Color(229, 232, 235),
                TextBoxBackground = new Color(23, 26, 30),
                TextBoxBackgroundSelected = new Color(38, 54, 64),
                CollectionBackgroundColor = new Color(36, 39, 43),
                ProgressNormal = new Color(98, 181, 140),
                Selection = new Color(213, 160, 76, 102),
                SelectionBorder = new Color(213, 160, 76),
                Statusbar = new StatusbarStyle
                {
                    PlayMode = new Color(43, 97, 69),
                    Failed = new Color(128, 51, 55),
                    Loading = new Color(52, 69, 82),
                },
            };

            style.DragWindow = new Color(64, 135, 177, 160);
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
                Background = new Color(241, 242, 244),
                LightBackground = new Color(226, 229, 233),
                DragWindow = new Color(60, 125, 166, 122),
                Foreground = new Color(38, 49, 61),
                ForegroundGrey = new Color(86, 97, 109),
                ForegroundDisabled = new Color(126, 135, 145),
                ForegroundViewport = new Color(255, 255, 255),
                BackgroundHighlighted = new Color(212, 220, 228),
                BorderHighlighted = new Color(154, 177, 194),
                BackgroundSelected = new Color(203, 221, 235),
                BorderSelected = new Color(57, 131, 173),
                BackgroundNormal = new Color(232, 234, 238),
                BorderNormal = new Color(194, 201, 208),
                TextColor = new Color(38, 49, 61),
                TextBoxBackground = new Color(255, 255, 255),
                TextBoxBackgroundSelected = new Color(229, 241, 250),
                CollectionBackgroundColor = new Color(248, 249, 250),
                ProgressNormal = new Color(63, 157, 114),
                Selection = new Color(184, 125, 42, 102),
                SelectionBorder = new Color(184, 125, 42),
                Statusbar = new StatusbarStyle
                {
                    PlayMode = new Color(66, 153, 108),
                    Failed = new Color(189, 81, 83),
                    Loading = new Color(100, 150, 180),
                },
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
