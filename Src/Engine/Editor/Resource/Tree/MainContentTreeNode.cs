

using System.IO;

namespace SE.Editor
{

    /// <summary>
    /// Content tree node used for main directories.
    /// </summary>
    public class MainContentTreeNode : ContentTreeNode
    {
        private FileSystemWatcher m_Watcher;

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
            m_Watcher = new FileSystemWatcher(path)
            {
                IncludeSubdirectories = true,
                EnableRaisingEvents = true,
                NotifyFilter = NotifyFilters.CreationTime | NotifyFilters.Size | NotifyFilters.FileName
                    | NotifyFilters.DirectoryName,
            };
            m_Watcher.Changed += OnEvent;
            m_Watcher.Created += OnEvent;
            m_Watcher.Deleted += OnEvent;
            m_Watcher.Renamed += OnEvent;
        }

        /// <inheritdoc />
        public override void OnDestroy()
        {
            // _watcher.EnableRaisingEvents = false;
            m_Watcher.Dispose();
            m_Watcher = null;

            base.OnDestroy();
        }

        /// <inheritdoc />
        protected override void BeginDragDrop()
        {
            // No drag for root nodes
        }
    }
}
