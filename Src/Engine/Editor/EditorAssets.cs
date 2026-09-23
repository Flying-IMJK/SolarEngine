namespace SE.Editor
{
    /// <summary>
    /// Contains the internal asset paths and shared fonts used by the managed editor UI.
    /// The native editor owns importing these assets before the managed editor starts.
    /// </summary>
    public static class EditorAssets
    {
        private static FontReference s_TitleFont = new();
        private static FontReference s_LargeFont = new();
        private static FontReference s_MediumFont = new();
        private static FontReference s_SmallFont = new();
        private static FontReference s_OutputLogFont = new();

        /// <summary>
        /// Segoe MDL2 glyphs used by the borderless editor window controls.
        /// </summary>
        public static class SegMDL2Icons
        {
            public static string Cancel = "\uE711";
            public static string ChromeMinimize = "\uE921";
            public static string ChromeMaximize = "\uE922";
            public static string ChromeRestore = "\uE923";
            public static string ChromeClose = "\uE8BB";
        }

        public static string AssetsPath = "Assets";
        public static string IconsPath = "Assets/Icon";
        public static string WindowIconsFont = "Assets/Fonts/SegMDL2";
        public static string PrimaryFont = "Assets/Fonts/Roboto-Regular";
        public static string InconsolataRegularFont = "Assets/Fonts/Roboto-Regular";
        public static string WindowIcon = "Assets/Icon/Engine_Icon";

        /// <summary>
        /// Gets or sets the title font for editor UI.
        /// </summary>
        public static FontReference TitleFont
        {
            get => new FontReference(s_TitleFont);
            set
            {
                // FontReference is a class in managed code, so copy it to preserve
                // the value semantics of the native EditorAssets implementation.
                if (value is null || !value.Font)
                    s_TitleFont.Font = GetDefaultFont();
                else
                    s_TitleFont = new FontReference(value);
            }
        }

        /// <summary>
        /// Gets or sets the large font for editor UI.
        /// </summary>
        public static FontReference LargeFont
        {
            get => new FontReference(s_LargeFont);
            set
            {
                if (value is null || !value.Font)
                    s_LargeFont.Font = GetDefaultFont();
                else
                    s_LargeFont = new FontReference(value);
            }
        }

        /// <summary>
        /// Gets or sets the medium font for editor UI.
        /// </summary>
        public static FontReference MediumFont
        {
            get => new FontReference(s_MediumFont);
            set
            {
                if (value is null || !value.Font)
                    s_MediumFont.Font = GetDefaultFont();
                else
                    s_MediumFont = new FontReference(value);
            }
        }

        /// <summary>
        /// Gets or sets the small font for editor UI.
        /// </summary>
        public static FontReference SmallFont
        {
            get => new FontReference(s_SmallFont);
            set
            {
                if (value is null || !value.Font)
                    s_SmallFont.Font = GetDefaultFont();
                else
                    s_SmallFont = new FontReference(value);
            }
        }

        /// <summary>
        /// Initializes the managed font references after native asset importing has completed.
        /// Loading remains asynchronous to match the native EditorAssets behavior.
        /// </summary>
        internal static void InitializeFonts()
        {
            s_TitleFont = new FontReference(GetDefaultFont(), 18.0f);
            s_LargeFont = new FontReference(GetDefaultFont(), 14.0f);
            s_MediumFont = new FontReference(GetDefaultFont(), 9.0f);
            s_SmallFont = new FontReference(GetDefaultFont(), 9.0f);
            s_OutputLogFont = new FontReference(GetConsoleFont(), 10.0f);
        }

        private static FontAsset GetDefaultFont()
        {
            return AssetContent.LoadAsyncInternal<FontAsset>(PrimaryFont);
        }

        private static FontAsset GetConsoleFont()
        {
            return AssetContent.LoadAsyncInternal<FontAsset>(InconsolataRegularFont);
        }
    }
}
