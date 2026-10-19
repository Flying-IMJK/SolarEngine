
using System;
using System.Collections.Generic;
using SE.GUI;
using SE.Editor.GUI;

namespace SE.Editor
{
    /// <summary>
    /// Content folder tree node.
    /// </summary>
    /// <seealso cref="TreeNode" />
    public class ContentTreeNode : TreeNode
    {
        private DragItems m_DragOverItems;
        private List<Rectangle> m_Highlights = new List<Rectangle>();

        /// <summary>
        /// The folder.
        /// </summary>
        protected ContentFolder m_Folder;

        /// <summary>
        /// Initializes a new instance of the <see cref="ContentTreeNode"/> class.
        /// </summary>
        /// <param name="parent">The parent node.</param>
        /// <param name="path">The folder path.</param>
        public ContentTreeNode(ContentTreeNode parent, string path)
            : this(parent, parent != null ? parent.GetFolderType() : ContentFolderType.Other, path)
        {
        }

        /// <summary>
        /// Whether this node can be deleted.
        /// </summary>
        public virtual bool GetCanDelete()
        {
            return true;
        }

        /// <summary>
        /// Whether this node can be duplicated.
        /// </summary>
        public virtual bool GetCanDuplicate()
        {
            return true;
        }

        /// <summary>
        /// Gets the content folder item.
        /// </summary>
        public ContentFolder GetFolder()
        {
            return m_Folder;
        }

        /// <summary>
        /// Gets the type of the folder.
        /// </summary>
        public ContentFolderType GetFolderType()
        {
            return m_Folder.FolderType;
        }

        /// <summary>
        /// Returns true if that folder can import/manage assets.
        /// </summary>
        /// <returns>True if can contain assets for project, otherwise false</returns>
        public bool GetCanHaveAssets()
        {
            return m_Folder.CanHaveAssets;
        }

        /// <summary>
        /// Gets the parent node.
        /// </summary>
        public ContentTreeNode ParentNode()
        {
            return Parent as ContentTreeNode;
        }

        /// <summary>
        /// Gets the folder path.
        /// </summary>
        public string GetPath()
        {
            return m_Folder.Path;
        }

        /// <summary>
        /// Gets the navigation button label.
        /// </summary>
        public virtual string GetNavButtonLabel()
        {
            return m_Folder.ShortName;
        }

        /// <summary>
        /// Shows the rename popup for the item.
        /// </summary>
        public void StartRenaming()
        {
            if (!m_Folder.CanRename)
            {
                return;
            }
            Editor.Instance?.Windows?.Content?.Rename(m_Folder);
        }

        /// <summary>
        /// Updates the query search filter.
        /// </summary>
        /// <param name="filterText">The filter text.</param>
        public void UpdateFilter(string filterText)
        {
            bool noFilter = filterText.Length <= 0;
            bool isThisVisible;
            if (noFilter)
            {
                m_Highlights.Clear();
                isThisVisible = true;
            }
            else
            {
                m_Highlights.Clear();
                isThisVisible = m_Folder.ShortName.Contains(filterText, StringComparison.OrdinalIgnoreCase);
            }

            bool isAnyChildVisible = false;
            for (int i = 0; i < m_Children.Count; i++)
            {
                if (m_Children[i] is ContentTreeNode child)
                {
                    child.UpdateFilter(filterText);
                    isAnyChildVisible |= child.Visible;
                }
            }

            bool isExpanded = isAnyChildVisible;
            if (isExpanded)
            {
                Expand(true);
            }
            else
            {
                Collapse(true);
            }

            Visible = IsRoot || isThisVisible || isAnyChildVisible;
        }

        /// <inheritdoc />
        public override void Draw()
        {
            base.Draw();

            if (m_Highlights.Count > 0)
            {
                Style style = Style.Current;
                Color color = style.ProgressNormal * 0.6f;
                for (int i = 0; i < m_Highlights.Count; i++)
                {
                    Render2D.FillRectangle(m_Highlights[i], color);
                }
            }
        }

        /// <inheritdoc />
        public override void OnDestroy()
        {
            m_Folder.Dispose();
            m_Folder = null;
            base.OnDestroy();
        }

        /// <inheritdoc />
        public override bool OnKeyDown(KeyboardKeys key)
        {
            if (IsFocused)
            {
                switch (key)
                {
                    case KeyboardKeys.F2:
                        StartRenaming();
                        return true;
                    case KeyboardKeys.Delete:
                        if (GetFolder().Exists && GetCanDelete())
                        {
                            Editor.Instance?.Windows?.Content?.Delete(GetFolder());
                        }

                        return true;
                }

                if (RootWindow().GetKey(KeyboardKeys.Control))
                {
                    switch (key)
                    {
                        case KeyboardKeys.D:
                            if (GetFolder().Exists && GetCanDuplicate())
                            {
                                Editor.Instance?.Windows?.Content?.Duplicate(GetFolder());
                            }

                            return true;
                    }
                }
            }

            return base.OnKeyDown(key);
        }

        private DragDropEffect GetDragEffect(DragData data)
        {
            if (data is DragDataFiles)
            {
                if (GetFolder().CanHaveAssets)
                {
                    return DragDropEffect.Copy;
                }
            }
            else if (m_DragOverItems?.HasValidDrag == true)
            {
                return DragDropEffect.Move;
            }

            return DragDropEffect.None;
        }

        private bool ValidateDragItem(ContentItem item)
        {
            ContentFolder target = GetFolder();
            return target.FolderType != ContentFolderType.Other &&
                   item.ParentFolder?.FolderType == target.FolderType &&
                   item != target && !item.Find(target);
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="ContentTreeNode"/> class.
        /// </summary>
        /// <param name="parent">The parent node.</param>
        /// <param name="type">The folder type.</param>
        /// <param name="path">The folder path.</param>
        protected ContentTreeNode(ContentTreeNode parent, ContentFolderType type, string path)
            : base(false, EditorIcons.FolderClosed32, EditorIcons.FolderOpen32)
        {
            m_Folder = new ContentFolder(type, path, this);
            Text = m_Folder.ShortName;
            if (parent != null)
            {
                GetFolder().ParentFolder = parent.GetFolder();
                Parent = parent;
            }

            IconColor = Style.Current.Foreground;
        }

        /// <inheritdoc />
        protected override DragDropEffect OnDragEnterHeader(DragData data)
        {
            m_DragOverItems ??= new DragItems(ValidateDragItem, Editor.Instance.AssetDatabase.Find);
            m_DragOverItems.OnDragEnter(data);
            return GetDragEffect(data);
        }

        /// <inheritdoc />
        protected override DragDropEffect OnDragMoveHeader(DragData data)
        {
            return GetDragEffect(data);
        }

        /// <inheritdoc />
        protected override void OnDragLeaveHeader()
        {
            m_DragOverItems?.OnDragLeave();
            base.OnDragLeaveHeader();
        }

        /// <inheritdoc />
        protected override DragDropEffect OnDragDropHeader(DragData data)
        {
            DragDropEffect result = DragDropEffect.None;
            if (data is DragDataFiles files && GetFolder().CanHaveAssets)
            {
                Editor.Instance.Importing.Import(files.Files, GetFolder());
                result = DragDropEffect.Copy;
                Expand();
            }
            else if (m_DragOverItems?.HasValidDrag == true)
            {
                Editor.Instance.AssetDatabase.Move(m_DragOverItems.Objects, GetFolder());
                result = DragDropEffect.Move;
                Expand();
            }
            m_DragOverItems?.OnDragDrop();
            return result;
        }

        /// <inheritdoc />
        protected override void BeginDragDrop()
        {
            DragData data = DragItems.GetDragData(GetFolder());
            DoDragDrop(data);
        }

        /// <inheritdoc />
        protected override void OnLongPress()
        {
            Select();
            StartRenaming();
        }
    }
}
