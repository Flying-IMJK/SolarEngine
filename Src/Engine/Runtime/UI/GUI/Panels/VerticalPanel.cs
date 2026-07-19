namespace SE.GUI
{
    /// <summary>
    /// Arranges its visible children from top to bottom.
    /// </summary>
    public class VerticalPanel : PanelWithMargins
    {
        public VerticalPanel()
        {
        }

        public VerticalPanel(Rectangle bounds)
            : base(bounds)
        {
        }

        protected override void OnLayoutChildren()
        {
            Rectangle content = ContentBounds;
            float top = content.Y;
            bool hasChildren = false;

            for (int index = 0; index < Children.Count; index++)
            {
                Control child = Children[index];
                if (!child.Visible)
                    continue;

                child.SetBounds(content.X, top, content.Width, child.Height);
                top += child.Height + Spacing;
                hasChildren = true;
            }

            if (AutoSize && hasChildren)
                Height = top - Spacing + Margin.Bottom;
        }
    }
}
