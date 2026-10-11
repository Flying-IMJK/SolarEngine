#nullable disable annotations

using System;
using System.Collections.Generic;
using System.IO;
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
        private DragItems _dragOverItems;
        private List<Rectangle> _highlights = new List<Rectangle>();

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
                _highlights.Clear();
                isThisVisible = true;
            }
            else
            {
                _highlights.Clear();
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

            if (_highlights.Count > 0)
            {
                Style style = Style.Current;
                Color color = style.ProgressNormal * 0.6f;
                for (int i = 0; i < _highlights.Count; i++)
                {
                    Render2D.FillRectangle(_highlights[i], color);
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
            else if (_dragOverItems?.HasValidDrag == true)
                return DragDropEffect.Move;

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
            _dragOverItems ??= new DragItems(ValidateDragItem, Editor.Instance.AssetDatabase.Find);
            _dragOverItems.OnDragEnter(data);
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
            _dragOverItems?.OnDragLeave();
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
            else if (_dragOverItems?.HasValidDrag == true)
            {
                Editor.Instance.AssetDatabase.Move(_dragOverItems.Objects, GetFolder());
                result = DragDropEffect.Move;
                Expand();
            }
            _dragOverItems?.OnDragDrop();
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

    /// <summary>
    /// Content tree node used for main directories.
    /// </summary>
    public class MainContentTreeNode : ContentTreeNode
    {
        private FileSystemWatcher _watcher;

        private void OnEvent(object sender, FileSystemEventArgs eventArgs)
        {
            Editor.Instance?.AssetDatabase?.OnDirectoryEvent(this, eventArgs);
        }

        /// <inheritdoc />
        public override bool GetCanDelete()
        {
            return false;
        }

        /// <inheritdoc />
        public override bool GetCanDuplicate()
        {
            return false;
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="MainContentTreeNode"/> class.
        /// </summary>
        /// <param name="parent">The parent project.</param>
        /// <param name="type">The folder type.</param>
        /// <param name="path">The folder path.</param>
        public MainContentTreeNode(ProjectTreeNode parent, ContentFolderType type, string path)
            : base(parent, type, path)
        {
            _watcher = new FileSystemWatcher(path)
            {
                IncludeSubdirectories = true,
                EnableRaisingEvents = true,
                NotifyFilter = NotifyFilters.CreationTime | NotifyFilters.Size | NotifyFilters.FileName | NotifyFilters.DirectoryName,
            };
            _watcher.Changed += OnEvent;
            _watcher.Created += OnEvent;
            _watcher.Deleted += OnEvent;
            _watcher.Renamed += OnEvent;
        }

        /// <inheritdoc />
        public override void OnDestroy()
        {
            // _watcher.EnableRaisingEvents = false;
            _watcher.Dispose();
            _watcher = null;

            base.OnDestroy();
        }

        /// <inheritdoc />
        protected override void BeginDragDrop()
        {
            // No drag for root nodes
        }
    }

    /// <summary>
    /// Root tree node for the project workspace.
    /// </summary>
    public class ProjectTreeNode : ContentTreeNode
    {
        /// <summary>
        /// The project.
        /// </summary>
        public ProjectInfo Project;

        /// <summary>
        /// The project content directory.
        /// </summary>
        public MainContentTreeNode Content;

        /// <summary>
        /// The project source code directory.
        /// </summary>
        public MainContentTreeNode Source;

        /// <summary>
        /// Initializes a new instance of the <see cref="ProjectTreeNode"/> class.
        /// </summary>
        public ProjectTreeNode(ProjectInfo project) : base(null, project.ProjectFolderPath)
        {
            Project = project;
            ProjectName = project.Name;
            ContentFolder folder = GetFolder();
            folder.FileName = project.Name;
            folder.ShortName = project.Name;
            Text = project.Name;
        }

        public string ProjectName { get; }

        /// <inheritdoc />
        public override string GetNavButtonLabel()
        {
            // return Project.Name;
            return ProjectName;
        }

        /// <inheritdoc />
        public override int CompareTo(Control other)
        {
            // Move the main game project to the top
            // if (Project.Name == Editor.Instance.Project.Name)
            // {
            //     return -1;
            // }
            return base.CompareTo(other);
        }

        /// <inheritdoc />
        protected override void BeginDragDrop()
        {
            // No drag for root nodes
        }
    }
}
