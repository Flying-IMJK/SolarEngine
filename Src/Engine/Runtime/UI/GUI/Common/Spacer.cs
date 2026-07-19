namespace SE.GUI
{
    /// <summary>
    /// Inserts visual-only blank space into a layout.
    /// </summary>
    public sealed class Spacer : ContainerControl
    {
        public Spacer()
            : this(100.0f, 100.0f)
        {
        }

        public Spacer(float width, float height)
            : base(new Rectangle(0.0f, 0.0f, width, height))
        {
            AutoFocus = false;
            IsScrollable = false;
            Enabled = false;
        }
    }
}
