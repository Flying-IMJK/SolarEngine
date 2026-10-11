#nullable disable annotations

using System;
using SE.GUI;

namespace SE.Editor.GUI
{
    public class ContextMenuItem : ContainerControl
    {
        /// <summary>
        /// Gets the parent context menu.
        /// </summary>
        public ContextMenu ParentContextMenu;

        /// <summary>
        /// Gets the minimum width of this item.
        /// </summary>
        public float MinimumWidth => GetMinimumWidth();

        /// <inheritdoc />
        public override void OnMouseEnter(Float2 location)
        {
            ParentContextMenu.HideChild();

            base.OnMouseEnter(location);
        }

        public ContextMenuItem()
        {
            AutoFocus = false;
            ParentContextMenu = null;
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="ContextMenuItem"/> class.
        /// </summary>
        /// <param name="parent">The parent context menu.</param>
        /// <param name="width">The initial width.</param>
        /// <param name="height">The initial height.</param>
        protected ContextMenuItem(ContextMenu parent, float width, float height)
            : base(0, 0, width, height)
        {
            AutoFocus = false;
            ParentContextMenu = parent;
        }

        protected virtual float GetMinimumWidth()
        {
            float width = 0;

            for (int i = 0; i < m_Children.Count; i++)
            {
                Control child = m_Children[i];
                if (child.Visible)
                {
                    width = Math.Max(width, child.Right + 4.0f);
                }
            }

            return width;
        }
    }
}
