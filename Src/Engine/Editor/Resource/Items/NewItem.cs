using System;

namespace SE.Editor
{

    /// <summary>
    /// Helper content item used to mock UI during creating new assets by <see cref="SE.Editor.GUI.ContentWindow"/>.
    /// </summary>
    public class NewItem : ContentItem
    {
        /// <summary>
        /// Gets the proxy object related to the created asset.
        /// </summary>
        public ContentOperate Proxy;

        /// <summary>
        /// Gets the argument passed to the proxy for the item creation. In most cases it is null.
        /// </summary>
        public IntPtr Argument;

        /// <summary>
        /// Initializes a new instance of the <see cref="NewItem"/> class.
        /// </summary>
        /// <param name="path">The path for the new item.</param>
        /// <param name="proxy">The content proxy object.</param>
        /// <param name="arg">The argument passed to the proxy for the item creation. In most cases it is null.</param>
        public NewItem(string path, ContentOperate proxy, IntPtr arg) : base(path)
        {
            Proxy = proxy;
            Argument = arg;
        }

        /// <inheritdoc />
        // public override string TypeDescription => "New";

        /// <inheritdoc />
        // protected override bool DrawShadow => true;

        /// <inheritdoc />
        public override void UpdateTooltipText()
        {
            // TooltipText = null;
        }

        public override ContentItemType ItemType => ContentItemType.Other;

        public override ContentItemSearchFilter SearchFilter => ContentItemSearchFilter.Other;

        public override SpriteHandle DefaultThumbnail => EditorIcons.Document128;
    }
}
