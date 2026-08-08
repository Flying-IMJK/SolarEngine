using System;
using System.Collections.Generic;
using System.IO;
using SE.GUI;

namespace SE.Editor
{
    /// <summary>
    /// Composes the managed Resource tree and content view for the editor Content
    /// window. The browser owns only display models; import and asset work cross
    /// the narrow ResourceInterop boundary when requested by an action.
    /// </summary>
    public sealed class ManagedContentBrowser : ContainerControl
    {
        private readonly ContentFolder m_RootFolder;
        private readonly MainContentTreeNode m_RootNode;
        private readonly ContentView m_ContentView;

        public ManagedContentBrowser(string rootPath)
            : base(new Rectangle(0, 0, 600, 400))
        {
            ArgumentException.ThrowIfNullOrWhiteSpace(rootPath);
            m_RootFolder = new ContentFolder(ContentFolderType.Content, rootPath);
            m_RootNode = AddChild(new MainContentTreeNode(m_RootFolder));
            m_ContentView = AddChild(new ContentView());
            m_ContentView.ItemActivated += OnItemActivated;
            m_RootNode.RefreshRequested += _ => Refresh();
            Refresh();
        }

        public ContentFolder RootFolder => m_RootFolder;
        public ContentTreeNode RootNode => m_RootNode;
        public ContentView ContentView => m_ContentView;
        public ContentFolder CurrentFolder { get; private set; } = null!;

        public void Refresh()
        {
            CurrentFolder = m_RootFolder;
            RebuildFolder(m_RootFolder, m_RootNode);
            ShowFolder(m_RootFolder);
        }

        public void ShowFolder(ContentFolder folder)
        {
            ArgumentNullException.ThrowIfNull(folder);
            CurrentFolder = folder;
            m_ContentView.ShowItems(folder.Children, m_ContentView.SortType, keepSelection: true);
        }

        protected override void OnLayoutChildren()
        {
            float treeWidth = MathF.Min(MathF.Max(180.0f, Width * 0.28f), MathF.Max(180.0f, Width - 120.0f));
            m_RootNode.SetBounds(0, 0, treeWidth, Height);
            m_ContentView.SetBounds(treeWidth + 1.0f, 0, MathF.Max(0, Width - treeWidth - 1.0f), Height);
        }

        private void OnItemActivated(ContentItem item)
        {
            if (item is ContentFolder folder)
            {
                if (folder.Node != null)
                    RebuildFolder(folder, folder.Node);
                ShowFolder(folder);
            }
        }

        private static void RebuildFolder(ContentFolder folder, ContentTreeNode node)
        {
            node.ClearNodes();
            foreach (ContentItem item in new List<ContentItem>(folder.Children))
            {
                item.Dispose();
            }
            if (!Directory.Exists(folder.Path))
                return;

            foreach (string path in Directory.EnumerateDirectories(folder.Path))
            {
                ContentFolder child = new(folder.FolderType, path);
                folder.AddChild(child);
                node.AddFolder(child);
            }

            foreach (string path in Directory.EnumerateFiles(folder.Path))
            {
                folder.AddChild(new FileItem(path));
            }
        }
    }
}
