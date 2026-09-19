
namespace SE.GUI
{
    /// <summary>
    /// Managed GUI style shared by controls that do not provide a local visual value.
    /// </summary>
    public sealed class Style
    {
        /// <summary>
        /// Gets or sets the process-wide managed GUI style.
        /// </summary>
        public static Style Current { get; set; } = new Style();


        /// <summary>
        /// Style for the Statusbar
        /// </summary>
        public struct StatusbarStyle
        {
            /// <summary>
            /// Color of the Statusbar when in Play Mode
            /// </summary>
            Color PlayMode;

            /// <summary>
            /// Color of the Statusbar when in loading state (e.g. when importing assets)
            /// </summary>
            Color Loading;

            /// <summary>
            /// Color of the Statusbar in its failed state (e.g. with compilation errors)
            /// </summary>
            Color Failed;
        };


        private FontReference m_FontTitle;
        private FontReference m_FontLarge;
        private FontReference m_FontMedium;
        private FontReference m_FontSmall;

        /// <summary>
        /// The font title.
        /// </summary>
        public Font FontTitle
        {
            get => m_FontTitle?.GetFont();
            set => m_FontTitle = new FontReference(value);
        }


        /// <summary>
        /// The font large.
        /// </summary>
        public Font FontLarge
        {
            get => m_FontLarge?.GetFont();
            set => m_FontLarge = new FontReference(value);
        }

        /// <summary>
        /// The font medium.
        /// </summary>
        public Font FontMedium
        {
            get => m_FontMedium?.GetFont();
            set => m_FontMedium = new FontReference(value);
        }

        /// <summary>
        /// The font small.
        /// </summary>
        public Font FontSmall
        {
            get => m_FontSmall?.GetFont();
            set => m_FontSmall = new FontReference(value);
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
        /// The foreground disabled.
        /// </summary>
        public Color ForegroundDisabled;

        /// <summary>
        /// The foreground color in viewports (usually have a dark background)
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
        /// The text box background color.
        /// </summary>
        public Color TextColor;

        /// <summary>
        /// The text box background color.
        /// </summary>
        public Color TextBoxBackground;

        /// <summary>
        /// The text box background selected color.
        /// </summary>
        public Color TextBoxBackgroundSelected;

        /// <summary>
        /// The collection background color.
        /// </summary>
        public Color CollectionBackgroundColor;

        /// <summary>
        /// The progress normal color.
        /// </summary>
        public Color ProgressNormal;

        /// <summary>
        /// The selection and drag drop highlights colors.
        /// </summary>
        public Color Selection;

        /// <summary>
        /// The selection and drag drop highlights border colors.
        /// </summary>
        public Color SelectionBorder;

        /// <summary>
        /// The status bar style
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
        /// The shared tooltip control used by the controls if no custom tooltip is provided.
        /// </summary>
        public Tooltip SharedTooltip;

    }
}
