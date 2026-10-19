// Copyright (c) Wojciech Figat. All rights reserved.

using SE.GUI;

namespace SE.Editor
{
    /// <summary>
    /// The vertical panel element.
    /// </summary>
    /// <seealso cref="SE.Editor.LayoutElement" />
    public class VerticalPanelElement : LayoutElementsContainer
    {
        /// <summary>
        /// The panel.
        /// </summary>
        public readonly VerticalPanel Panel = new VerticalPanel
        {
            Pivot = Float2.Zero,
        };

        /// <inheritdoc />
        public override ContainerControl ContainerControl => Panel;
    }
}
