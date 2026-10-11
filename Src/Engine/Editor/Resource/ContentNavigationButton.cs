#nullable disable annotations

using SE.Editor.GUI;
using SE.GUI;

namespace SE.Editor
{
    /// <summary>
    /// A navigation button for <see cref="GUI.ContentWindow"/>.
    /// </summary>
    public class ContentNavigationButton : NavigationButton
    {
        private DragItems _dragOverItems;

        /// <summary>
        /// Gets the target node.
        /// </summary>
        public ContentTreeNode TargetNode;

        public ContentNavigationButton()
            : base(0, 0, 0)
        {
            TargetNode = null;
            Text = string.Empty;
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="ContentNavigationButton"/> class.
        /// </summary>
        /// <param name="targetNode">The target node.</param>
        /// <param name="x">The x position.</param>
        /// <param name="y">The y position.</param>
        /// <param name="height">The height.</param>
        public ContentNavigationButton(ContentTreeNode targetNode, float x, float y, float height)
            : base(x, y, height)
        {
            TargetNode = targetNode;
            Text = targetNode.GetNavButtonLabel();
        }

        /// <inheritdoc />
        public override DragDropEffect OnDragEnter(ref Float2 location, DragData data)
        {
            base.OnDragEnter(ref location, data);
            _dragOverItems ??= new DragItems(ValidateDragItem, Editor.Instance.AssetDatabase.Find);
            _dragOverItems.OnDragEnter(data);
            DragDropEffect result = GetDragEffect(data);
            m_ValidDragOver = result != DragDropEffect.None;
            return result;
        }

        /// <inheritdoc />
        public override DragDropEffect OnDragMove(ref Float2 location, DragData data)
        {
            base.OnDragMove(ref location, data);

            return GetDragEffect(data);
        }

        /// <inheritdoc />
        public override void OnDragLeave()
        {
            base.OnDragLeave();

            if (_dragOverItems != null)
            {
                _dragOverItems.OnDragLeave();
            }
            m_ValidDragOver = false;
        }

        /// <inheritdoc />
        public override DragDropEffect OnDragDrop(ref Float2 location, DragData data)
        {
            DragDropEffect result = DragDropEffect.None;
            base.OnDragDrop(ref location, data);

            // Check if drop element or files
            if (data is DragDataFiles files && TargetNode.GetCanHaveAssets())
            {
                Editor.Instance.Importing.Import(files.Files, TargetNode.GetFolder());
                result = DragDropEffect.Copy;
            }
            else if (_dragOverItems?.HasValidDrag == true)
            {
                Editor.Instance.AssetDatabase.Move(_dragOverItems.Objects, TargetNode.GetFolder());
                result = DragDropEffect.Move;
            }

            if (_dragOverItems != null)
            {
                _dragOverItems.OnDragDrop();
            }
            m_ValidDragOver = false;

            return result;
        }

        /// <inheritdoc />
        protected override void OnClick()
        {
            // Navigate
            Editor.Instance.Windows.ContentWin.Navigate(TargetNode);

            base.OnClick();
        }

        private DragDropEffect GetDragEffect(DragData data)
        {
            if (data is DragDataFiles)
            {
                if (TargetNode.GetCanHaveAssets())
                {
                    return DragDropEffect.Copy;
                }
            }
            else if (_dragOverItems?.HasValidDrag == true)
                return DragDropEffect.Move;

            return DragDropEffect.None;
        }

        private bool ValidateDragItem(ContentItem item)
        {
            ContentFolder target = TargetNode.GetFolder();
            return target.FolderType != ContentFolderType.Other &&
                   item.ParentFolder?.FolderType == target.FolderType &&
                   item != target && !item.Find(target);
        }
    }
}
