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
            // The shared managed palette also styles Runtime GUI controls.
            Style style = Style.CreateDefaultStyle();
            ApplySharedAssets(style);
            return style;
        }

        /// <summary>
        /// Creates the default light editor style.
        /// </summary>
        public Style CreateLightStyle()
        {
            Style style = Style.CreateLightStyle();
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
