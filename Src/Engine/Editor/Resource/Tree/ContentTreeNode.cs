using System;
using System.Collections.Generic;
using System.IO;
using SE.Editor.GUI;

namespace SE.Editor
{
    /// <summary>
    /// Managed tree representation of a content folder. Filesystem events are
    /// converted into model refresh requests rather than exposing native pointers.
    /// </summary>
    public class ContentTreeNode : TreeNode, IContentItemOwner
    {
        public ContentTreeNode(ContentFolder folder)
        {
            Folder = folder ?? throw new ArgumentNullException(nameof(folder));
            Folder.Node = this;
            Folder.AddReference(this);
            Text = folder.DisplayName;
        }

        public ContentFolder Folder { get; }
        public string Path => Folder.Path;
        public virtual bool CanDelete => Folder.ParentFolder != null;
        public virtual bool CanDuplicate => Folder.ParentFolder != null;
        public virtual bool CanHaveAssets => Folder.CanHaveAssets;
        public event Action<ContentTreeNode>? RenameRequested;
        public event Action<ContentTreeNode>? RefreshRequested;

        public void StartRenaming()
        {
            if (Folder.CanRename)
                RenameRequested?.Invoke(this);
        }

        public void UpdateFilter(string filterText)
        {
            bool noFilter = string.IsNullOrWhiteSpace(filterText);
            bool ownMatch = noFilter || Text.Contains(filterText, StringComparison.OrdinalIgnoreCase);
            bool childMatch = false;
            foreach (TreeNode node in Nodes)
            {
                if (node is not ContentTreeNode child)
                    continue;
                child.UpdateFilter(filterText);
                childMatch |= child.Visible;
            }

            Visible = ownMatch || childMatch;
            if (!noFilter && childMatch)
                Expand(noAnimation: true);
        }

        public ContentTreeNode AddFolder(ContentFolder folder)
        {
            ContentTreeNode node = new(folder);
            AddNode(node);
            return node;
        }

        public void Refresh() => RefreshRequested?.Invoke(this);

        public void OnItemDeleted(ContentItem item)
        {
            if (ReferenceEquals(item, Folder))
                Dispose();
        }

        public void OnItemRenamed(ContentItem item)
        {
            if (!ReferenceEquals(item, Folder))
                return;
            Text = Folder.DisplayName;
            Refresh();
        }

        public void OnItemReimported(ContentItem item)
        {
            if (ReferenceEquals(item, Folder))
                Refresh();
        }

        public void OnItemDispose(ContentItem item)
        {
            if (ReferenceEquals(item, Folder))
                Dispose();
        }

        protected override void OnDispose()
        {
            Folder.RemoveReference(this);
            if (ReferenceEquals(Folder.Node, this))
                Folder.Node = null;
            base.OnDispose();
        }
    }

    public sealed class MainContentTreeNode : ContentTreeNode
    {
        private readonly FileSystemWatcher? m_Watcher;

        public MainContentTreeNode(ContentFolder folder, bool watchFileSystem = false)
            : base(folder)
        {
            if (!watchFileSystem || !Directory.Exists(folder.Path))
                return;

            m_Watcher = new FileSystemWatcher(folder.Path)
            {
                IncludeSubdirectories = true,
                EnableRaisingEvents = true,
                NotifyFilter = NotifyFilters.FileName | NotifyFilters.DirectoryName | NotifyFilters.LastWrite,
            };
            m_Watcher.Changed += OnFileSystemChanged;
            m_Watcher.Created += OnFileSystemChanged;
            m_Watcher.Deleted += OnFileSystemChanged;
            m_Watcher.Renamed += OnFileSystemChanged;
        }

        public override bool CanDelete => false;
        public override bool CanDuplicate => false;

        protected override void OnDispose()
        {
            if (m_Watcher != null)
            {
                m_Watcher.EnableRaisingEvents = false;
                m_Watcher.Dispose();
            }
            base.OnDispose();
        }

        private void OnFileSystemChanged(object sender, FileSystemEventArgs args)
        {
            _ = sender;
            _ = args;
            Refresh();
        }
    }

    public sealed class ProjectTreeNode : ContentTreeNode
    {
        public ProjectTreeNode(ContentFolder folder, string projectName)
            : base(folder)
        {
            ProjectName = projectName ?? throw new ArgumentNullException(nameof(projectName));
            Text = ProjectName;
        }

        public string ProjectName { get; }
        public override bool CanDelete => false;
        public override bool CanDuplicate => false;
    }
}
