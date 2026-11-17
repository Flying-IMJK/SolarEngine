// Copyright (c) Wojciech Figat. All rights reserved.

using SE.GUI;

namespace SE.Editor
{
    /// <summary>
    /// The button element.
    /// </summary>
    /// <seealso cref="SE.Editor.LayoutElement" />
    public class ButtonElement : LayoutElement
    {
        /// <summary>
        /// The button.
        /// </summary>
        public readonly Button Button = new Button
        {
            Width = 120.0f,
            Height = SE.GUI.Button.DefaultHeight,
        };

        /// <inheritdoc />
        public override Control Control => Button;
    }
}
