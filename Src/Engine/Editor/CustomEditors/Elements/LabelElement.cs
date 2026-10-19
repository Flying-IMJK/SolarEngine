// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using SE.Editor.GUI;
using SE.GUI;

namespace SE.Editor
{
    /// <summary>
    /// The label element.
    /// </summary>
    /// <seealso cref="SE.Editor.LayoutElement" />
    public class LabelElement : LayoutElement
    {
        private Action<ContextMenu> m_CustomContextualOptions;

        /// <summary>
        /// The label.
        /// </summary>
        public readonly ClickableLabel Label;

        /// <summary>
        /// Initializes a new instance of the <see cref="CheckBoxElement"/> class.
        /// </summary>
        public LabelElement()
        {
            Label = new ClickableLabel
            {
                Size = new Float2(100, 18),
                HorizontalAlignment = TextAlignment.Near,
            };
            // TODO: auto height for label
        }

        /// <summary>
        /// Adds a simple context menu with utility to copy label text. Can be extended with more options.
        /// </summary>
        public LabelElement AddCopyContextMenu(Action<ContextMenu> customOptions = null)
        {
            Label.RightClick += OnRightClick;
            m_CustomContextualOptions = customOptions;
            return this;
        }

        private void OnRightClick()
        {
            ContextMenu menu = new ContextMenu();
            menu.AddButton("Copy text").Clicked += OnCopyText;
            m_CustomContextualOptions?.Invoke(menu);
            menu.Show(Label, Label.PointFromScreen(Input.MouseScreenPosition));
        }

        private void OnCopyText()
        {
            Clipboard.Text = Label.Text;
        }

        /// <inheritdoc />
        public override Control Control => Label;
    }
}
