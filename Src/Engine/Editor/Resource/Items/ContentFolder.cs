

using System;
using System.Collections.Generic;
using System.IO;
using System.Text;

using SE.Editor.GUI;
using SE.GUI;

namespace SE.Editor
{
    /// <summary>
    /// Types of content directories.
    /// </summary>
    public enum ContentFolderType
    {
        /// <summary>
        /// The directory with assets.
        /// </summary>
        Content,

        /// <summary>
        /// The directory with source files.
        /// </summary>
        Source,

        /// <summary>
        /// The other type of directory.
        /// </summary>
        Other,
    };


    public class ContentFolder : ContentItem
    {
        private DragItems m_DragOverItems;
        private bool m_ValidDragOver;

        /// <summary>Gets the type of the folder.</summary>
        public ContentFolderType FolderType;

        /// <summary>Returns true if that folder can import/manage scripts.</summary>
        // public bool CanHaveScripts => FolderType == ContentFolderType.Source;

        /// <summary>Returns true if that folder can import/manage assets.</summary>
        public bool CanHaveAssets => FolderType == ContentFolderType.Content;

        /// <summary>Gets the content node.</summary>
        public ContentTreeNode Node;

        /// <summary>The subitems of this folder.</summary>
        public List<ContentItem> Children = new List<ContentItem>();

        /// <summary>
        /// Initializes a new instance of the <see cref="ContentFolder"/> class.
        /// </summary>
        /// <param name="type">The folder type.</param>
        /// <param name="path">The path to the item.</param>
        /// <param name="node">The folder parent node.</param>
        public ContentFolder(ContentFolderType type, string path, ContentTreeNode node)
            : base(path)
        {
            FolderType = type;
            Node = node;
            ShortName = System.IO.Path.GetFileNameWithoutExtension(path);
        }

        /// <summary>Tries to find child element with given path.</summary>
        /// <param name="path">Element path to find.</param>
        /// <returns>Found element or null.</returns>
        public ContentItem FindChild(string path)
        {
            for (int i = 0; i < Children.Count; i++)
            {
                if (Children[i].Path == path)
                {
                    return Children[i];
                }
            }

            return null;
        }

        /// <summary>Check if folder contains child element with given path.</summary>
        /// <param name="path">Element path to find.</param>
        /// <returns>True if contains that element, otherwise false.</returns>
        public bool ContainsChild(string path)
        {
            return FindChild(path) != null;
        }

        /// <inheritdoc />
        public override void UpdatePath(string value)
        {
            base.UpdatePath(value);

            ShortName = System.IO.Path.GetFileName(value);
            Node.Text = ShortName;
        }

        /// <inheritdoc />
        public override ContentItem Find(string path)
        {
            if (Path == path)
            {
                return this;
            }

            for (int i = 0; i < Children.Count; i++)
            {
                ContentItem result = Children[i].Find(path);
                if (result != null)
                {
                    return result;
                }
            }

            return null;
        }

        /// <inheritdoc />
        public override bool Find(ContentItem item)
        {
            if (item == this)
            {
                return true;
            }

            for (int i = 0; i < Children.Count; i++)
            {
                if (Children[i].Find(item))
                {
                    return true;
                }
            }

            return false;
        }

        /// <inheritdoc />
        public override ContentItem Find(Guid id)
        {
            for (int i = 0; i < Children.Count; i++)
            {
                ContentItem result = Children[i].Find(id);
                if (result != null)
                {
                    return result;
                }
            }

            return null;
        }

        /// <inheritdoc />
        public override int CompareTo(Control other)
        {
            if (other is ContentItem otherItem)
            {
                if (!otherItem.IsFolder)
                {
                    return -1;
                }

                return string.CompareOrdinal(ShortName, otherItem.ShortName);
            }

            return base.CompareTo(other);
        }

        /// <inheritdoc />
        public override void Draw()
        {
            base.Draw();

            if (IsDragOver() && m_ValidDragOver)
            {
                Style style = Style.Current;
                Rectangle bounds = new Rectangle(Float2.Zero, Size);
                Render2D.FillRectangle(bounds, style.Selection);
                Render2D.DrawRectangle(bounds, style.SelectionBorder, 1.0f);
            }
        }

        /// <inheritdoc />
        public override DragDropEffect OnDragEnter(ref Float2 location, DragData data)
        {
            base.OnDragEnter(ref location, data);

            if (data is DragDataFiles && CanHaveAssets)
            {
                m_ValidDragOver = true;
                return DragDropEffect.Copy;
            }
            m_DragOverItems ??= new DragItems(ValidateDragItem, Editor.Instance.AssetDatabase.Find);
            m_DragOverItems.OnDragEnter(data);
            m_ValidDragOver = m_DragOverItems.HasValidDrag;
            return m_DragOverItems.Effect;
        }

        /// <inheritdoc />
        public override DragDropEffect OnDragMove(ref Float2 location, DragData data)
        {
            base.OnDragMove(ref location, data);

            if (data is DragDataFiles && CanHaveAssets)
            {
                return DragDropEffect.Copy;
            }
            return m_DragOverItems?.Effect ?? DragDropEffect.None;
        }

        /// <inheritdoc />
        public override DragDropEffect OnDragDrop(ref Float2 location, DragData data)
        {
            DragDropEffect result = base.OnDragDrop(ref location, data);

            if (data is DragDataFiles files && CanHaveAssets)
            {
                Editor.Instance.Importing.Import(files.Files, this);
                result = DragDropEffect.Copy;
            }
            else if (m_DragOverItems?.HasValidDrag == true)
            {
                Editor.Instance.AssetDatabase.Move(m_DragOverItems.Objects, this);
                result = DragDropEffect.Move;
            }

            if (m_DragOverItems != null)
            {
                m_DragOverItems.OnDragDrop();
            }
            m_ValidDragOver = false;

            return result;
        }

        /// <inheritdoc />
        public override void OnDragLeave()
        {
            if (m_DragOverItems != null)
            {
                m_DragOverItems.OnDragLeave();
            }
            m_ValidDragOver = false;

            base.OnDragLeave();
        }

        private bool ValidateDragItem(ContentItem item)
        {
            return FolderType != ContentFolderType.Other &&
                   item.ParentFolder?.FolderType == FolderType &&
                   item != this && !item.Find(this);
        }

        /// <inheritdoc />
        protected override void OnBuildTooltipText(StringBuilder sb)
        {
            // Native tooltip text body is commented out.
        }

        /// <inheritdoc />
        protected override void OnParentFolderChanged()
        {
            if (ParentFolder != null)
            {
                Node.Parent = ParentFolder.Node;
            }
            else
            {
                Node.Parent = null;
            }

            base.OnParentFolderChanged();
        }

        /// <inheritdoc />
        public override ContentItemSearchFilter SearchFilter => ContentItemSearchFilter.Other;

        /// <inheritdoc />
        public override ContentItemType ItemType => ContentItemType.Folder;

        /// <inheritdoc />
        public override bool CanRename => ParentFolder != null && Node is not MainContentTreeNode;

        /// <inheritdoc />
        public override bool CanDrag => ParentFolder != null;

        /// <inheritdoc />
        public override bool Exists => Directory.Exists(Path);

        /// <inheritdoc />
        public override SpriteHandle DefaultThumbnail => EditorIcons.Folder128;

        /// <inheritdoc />
        public override bool HasDefaultThumbnail => true;
    }
}
