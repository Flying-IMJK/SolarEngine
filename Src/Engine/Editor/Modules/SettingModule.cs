using SE.GUI;

namespace SE.Editor
{
    /// <summary>
    /// Configures managed editor fonts and GUI style after the native settings
    /// module has prepared the editor assets and icon atlas.
    /// </summary>
    public sealed class SettingModule : EditorModule
    {
        public SettingModule(Editor editor)
            : base(editor)
        {
        }

        internal override int Order => -100;

        public override void OnInit()
        {
            EditorAssets.InitializeFonts();

            // Match the currently selected native default until editor theme
            // persistence is exposed to the managed settings module.
            Style.Current = CreateLightStyle();
        }

        /// <summary>
        /// Creates the default dark editor style.
        /// </summary>
        public Style CreateDefaultStyle()
        {
            Style style = new Style
            {
                Background = new Color(0.109804f, 1.0f, 0.109804f, 0.109804f),
                LightBackground = new Color(0.176471f, 1.0f, 0.176471f, 0.188235f),
                Foreground = new Color(1.0f, 1.0f, 1.0f, 1.0f),
                ForegroundGrey = new Color(0.662745f, 1.0f, 0.662745f, 0.701961f),
                ForegroundDisabled = new Color(0.470588f, 1.0f, 0.470588f, 0.513725f),
                ForegroundViewport = new Color(1.0f, 1.0f, 1.0f, 1.0f),
                BackgroundHighlighted = new Color(0.329412f, 1.0f, 0.329412f, 0.360784f),
                BorderHighlighted = new Color(0.415686f, 1.0f, 0.415686f, 0.458824f),
                BackgroundSelected = new Color(0.478431f, 1.0f, 0.0f, 0.8f),
                BorderSelected = new Color(0.592157f, 1.0f, 0.109804f, 0.917647f),
                BackgroundNormal = new Color(0.247059f, 1.0f, 0.247059f, 0.27451f),
                BorderNormal = new Color(0.329412f, 1.0f, 0.329412f, 0.360784f),
                TextBoxBackground = new Color(0.2f, 1.0f, 0.2f, 0.215686f),
                TextBoxBackgroundSelected = new Color(0.247059f, 1.0f, 0.247059f, 0.27451f),
                CollectionBackgroundColor = new Color(0.8f, 0.078431f, 0.8f, 0.8f),
                ProgressNormal = new Color(0.827451f, 1.0f, 0.039216f, 0.156863f),
                Selection = Color.Orange * 0.4f,
                SelectionBorder = Color.Orange,
                Statusbar = new Style.StatusbarStyle
                {
                    PlayMode = new Color(0.568627f, 1.0f, 0.184314f, 0.207843f),
                    Failed = new Color(0.141176f, 1.0f, 0.611765f, 0.141176f),
                    Loading = new Color(0.176471f, 1.0f, 0.176471f, 0.188235f),
                },
            };

            style.TextColor = style.Foreground;
            style.DragWindow = style.BackgroundSelected * 0.7f;
            ApplySharedAssets(style);
            return style;
        }

        /// <summary>
        /// Creates the default light editor style.
        /// </summary>
        public Style CreateLightStyle()
        {
            Style style = new Style
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

            ApplySharedAssets(style);
            return style;
        }

        private static void ApplySharedAssets(Style style)
        {
            style.FontTitle = EditorAssets.TitleFont.GetFont();
            style.FontLarge = EditorAssets.LargeFont.GetFont();
            style.FontMedium = EditorAssets.MediumFont.GetFont();
            style.FontSmall = EditorAssets.SmallFont.GetFont();

            // These properties are generated from the native EditorIcons type.
            // The native settings module owns loading and uploading their atlas.
            style.ArrowDown = EditorIcons.ArrowDown12;
            style.ArrowRight = EditorIcons.ArrowRight12;
            style.Search = EditorIcons.Search12;
            style.Settings = EditorIcons.Settings12;
            style.Cross = EditorIcons.Cross16;
            style.CheckBoxIntermediate = EditorIcons.CheckBoxIntermediate12;
            style.CheckBoxTick = EditorIcons.CheckBoxTick12;
            style.StatusBarSizeGrip = EditorIcons.WindowDrag12;
            style.Translate = EditorIcons.Translate32;
            style.Rotate = EditorIcons.Rotate32;
            style.Scale = EditorIcons.Scale32;
            style.Scalar = EditorIcons.Scalar32;
        }
    }
}
