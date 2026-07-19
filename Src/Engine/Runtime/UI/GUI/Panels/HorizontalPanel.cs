namespace SE.GUI
{
    /// <summary>
    /// Arranges its visible children from left to right.
    /// </summary>
    public class HorizontalPanel : PanelWithMargins
    {
        public HorizontalPanel()
        {
        }

        public HorizontalPanel(Rectangle bounds)
            : base(bounds)
        {
        }

        protected override void OnLayoutChildren()
        {
            Rectangle content = ContentBounds;
            float left = content.X;
            bool hasChildren = false;

            for (int index = 0; index < Children.Count; index++)
            {
                Control child = Children[index];
                if (!child.Visible)
                    continue;

                child.SetBounds(left, content.Y, child.Width, content.Height);
                left += child.Width + Spacing;
                hasChildren = true;
            }

            if (AutoSize && hasChildren)
                Width = left - Spacing + Margin.Right;
        }
    }
}
