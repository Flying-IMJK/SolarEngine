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

        public Font? FontTitle { get; set; }
        public Font? FontLarge { get; set; }
        public Font? FontMedium { get; set; }
        public Font? FontSmall { get; set; }

        public Color Background { get; set; } = new Color(37, 37, 38);
        public Color BackgroundNormal { get; set; } = new Color(45, 45, 48);
        public Color BackgroundHighlighted { get; set; } = new Color(62, 62, 68);
        public Color BackgroundSelected { get; set; } = new Color(30, 30, 33);
        public Color Foreground { get; set; } = Color.White;
        public Color ForegroundDisabled { get; set; } = new Color(153, 153, 153);
    }
}
