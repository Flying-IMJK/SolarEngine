
using SE.GUI;

namespace SE.Editor.GUI
{
    /// <summary>
    /// A navigation bar control. Shows the current location path with UI buttons to navigate around.
    /// </summary>
    public class NavigationBar : Panel
    {
        /// <summary>
        /// The default buttons margin.
        /// </summary>
        public const float DefaultButtonsMargin = 2.0f;

        public NavigationBar() : base(ScrollBars.Horizontal)
        {
        }

        /// <summary>
        /// Updates the bar bounds and positions it after the last toolstrip button. Ensures to fit the toolstrip area
        /// (navigation bar horizontal scroll bar can be used to view the full path).
        /// </summary>
        /// <param name="toolstrip">The toolstrip.</param>
        public void UpdateBounds(ToolStrip toolstrip)
        {
            if (toolstrip == null)
            {
                return;
            }
            ToolStripButton lastToolstripButton = toolstrip.LastButton;
            Float2 parentSize = Parent.Size;
            Bounds = new Rectangle(lastToolstripButton.Right + 8.0f, 0,
                parentSize.X - X - 8.0f, toolstrip.Height);
        }

        /// <inheritdoc />
        protected override void Arrange()
        {
            base.Arrange();

            // Arrange buttons
            float x = DefaultButtonsMargin;
            for (int i = 0; i < m_Children.Count; i++)
            {
                Control child = m_Children[i];
                if (child.IsScrollable)
                {
                    child.X = x;
                    x += child.Width + DefaultButtonsMargin;
                }
            }
        }
    }

}
