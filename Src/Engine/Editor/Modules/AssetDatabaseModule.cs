using SE.Interop;

using System;
using System.Collections.Generic;
using System.IO;

namespace SE.Editor
{
    /// <summary>
    /// Managed asset database for content windows and workspace operations.
    /// </summary>
    public sealed class AssetDatabaseModule : EditorModule
    {
        private readonly object m_DirtyNodesLock = new();
        private readonly HashSet<MainContentTreeNode> m_DirtyNodes = new();
        private readonly object m_ImportedPathsLock = new();
        private readonly Queue<string> m_ImportedPaths = new();
        private bool m_EnableEvents;
        private bool m_IsDuringFastSetup;
        private bool m_RebuildFlag;
        private int m_ItemsCreated;
        private int m_ItemsDeleted;

        public ProjectTreeNode ProjectNode { get; private set; }
        public List<ProjectTreeNode> Projects { get; } = new();
        public List<ContentOperate> Operate { get; } = new(128);

        public event Action<ContentItem> ItemAdded;
        public event Action<ContentItem> ItemRemoved;
        public event Action WorkspaceModified;
        public event Action WorkspaceRebuilding;
        public event Action WorkspaceRebuilt;

        public int ItemsCreated => m_ItemsCreated;
        public int ItemsDeleted => m_ItemsDeleted;
        public int GetItemsCreated() => m_ItemsCreated;
        public int GetItemsDeleted() => m_ItemsDeleted;
        internal override int Order => -80;

        public AssetDatabaseModule(Editor editor) : base(editor)
        {
        }

        /// <summary>
        /// Gets the workspace node for a project.
        /// </summary>
        public ProjectTreeNode GetProjectWorkspace(ProjectInfo project)
        {
            foreach (ProjectTreeNode workspace in Projects)
            {
                if (ReferenceEquals(workspace.Project, project))
                {
                    return workspace;
                }
            }
            return null;
        }

        public ContentOperate GetProxy(ContentItem item)
        {
            if (item != null)
            {
                foreach (ContentOperate proxy in Operate)
                {
                    if (proxy.IsProxyFor(item))
                    {
                        return proxy;
                    }
                }
            }
            return null;
        }

        public ContentOperate GetProxy(Type assetType)
        {
            if (assetType != null)
            {
                foreach (ContentOperate proxy in Operate)
                {
                    if (proxy.IsProxyFor(assetType))
                    {
                        return proxy;
                    }
                }
            }
            return null;
        }

        public ContentOperate GetProxy<T>() where T : Asset
        {
            return GetProxy(typeof(T));
        }

        public ContentOperate GetProxyByExtension(string extension)
        {
            if (string.IsNullOrEmpty(extension))
            {
                return null;
            }

            foreach (ContentOperate proxy in Operate)
            {
                if (proxy.FileExtension == extension)
                {
                    return proxy;
                }
            }
            return null;
        }

        public ContentOperate GetProxy(string extension)
        {
            return GetProxyByExtension(extension);
        }

        public AssetOperate GetAssetOperate(Type assetType, string path)
        {
            foreach (ContentOperate proxy in Operate)
            {
                if (proxy is AssetOperate assetProxy && assetProxy.AcceptsAsset(assetType, path))
                {
                    return assetProxy;
                }
            }
            return null;
        }

        public AssetOperate GetAssetVirtualOperate(string path)
        {
            foreach (ContentOperate proxy in Operate)
            {
                if (proxy is AssetOperate assetProxy && assetProxy.IsVirtual && path.EndsWith(assetProxy.FileExtension, StringComparison.Ordinal))
                {
                    return assetProxy;
                }
            }
            return null;
        }

        public AssetOperate GetAssetVirtualOpreate(string path)
        {
            return GetAssetVirtualOperate(path);
        }

        public void RefreshFolder(ContentItem item, bool checkSubDirs)
        {
            ContentFolder folder = item.IsFolder ? item as ContentFolder : item.ParentFolder;
            if (folder != null)
            {
                LoadFolder(folder.Node, checkSubDirs);
            }
        }

        public ContentItem Find(string path)
        {
            if (string.IsNullOrEmpty(path))
            {
                return null;
            }

            string normalizedPath = FileSystem.NormalizePath(path);
            foreach (ProjectTreeNode project in Projects)
            {
                ContentItem result = project.GetFolder().Find(normalizedPath);
                if (result != null)
                {
                    return result;
                }
            }
            return null;
        }

        public ContentItem Find(Guid id)
        {
            if (id == Guid.Empty)
            {
                return null;
            }

            foreach (ProjectTreeNode project in Projects)
            {
                ContentItem result = project.GetFolder().Find(id);
                if (result != null)
                {
                    return result;
                }
            }
            return null;
        }

        public AssetItem FindAsset(Guid id)
        {
            if (id == Guid.Empty)
            {
                return null;
            }

            foreach (ProjectTreeNode project in Projects)
            {
                if (project.Content != null)
                {
                    AssetItem result = project.Content.GetFolder().Find(id) as AssetItem;
                    if (result != null)
                    {
                        return result;
                    }
                }
            }
            return null;
        }

        public void Move(IEnumerable<ContentItem> items, ContentFolder newParent)
        {
            foreach (ContentItem item in items)
            {
                Move(item, newParent);
            }
        }

        public void Move(ContentItem item, ContentFolder newParent)
        {
            Debug.Assert(item != null, "Item cannot be null.");
            Debug.Assert(newParent != null, "New parent cannot be null.");
            if (item.ParentFolder == newParent)
            {
                return;
            }

            string target = $"{newParent.Path}/{item.ShortName + Path.GetExtension(item.Path)}";
            Move(item, target);
        }

        public void Move(ContentItem item, string newPath)
        {
            Debug.Assert(item != null && !string.IsNullOrEmpty(newPath), "Item and target path must be valid.");

            if (item.IsFolder && Directory.Exists(newPath))
            {
                return;
            }

            ContentFolder newParent = Find(Path.GetDirectoryName(newPath)) as ContentFolder;
            if (newParent == null)
            {
                return;
            }

            if (item is ContentFolder folder)
            {
                string oldPath = item.Path;
                try
                {
                    Directory.CreateDirectory(newPath);
                }
                catch (Exception ex)
                {
                    Debug.LogWarning(ex);
                    Debug.LogError(string.Format("Cannot move folder \'{0}\' to \'{1}\'", oldPath, newPath));
                    return;
                }
   
                item.UpdatePath(newPath);
                for (int i = 0; i < folder.Children.Count; i++)
                {
                    UpdateAssetNewNameTree(folder.Children[i]);
                }


                try
                {
                    Directory.Delete(oldPath, true);
                }
                catch (Exception ex)
                {
                    Debug.LogWarning(ex);
                    Debug.LogError(string.Format("Cannot remove folder \'{0}\'", oldPath));
                }
            }
            else if (!RenameAsset(item, newPath))
            {
                return;
            }

            item.ParentFolder?.Node.SortChildren();
            item.ParentFolder = newParent;
            if (m_EnableEvents)
            {
                WorkspaceModified?.Invoke();
            }
        }

        public void Copy(ContentItem item, string targetPath)
        {
            if (item == null || !item.Exists)
            {
                return;
            }

            string sourcePath = item.Path;

            if (item.IsFolder)
            {
                ContentFolder folder = (ContentFolder)item;

                // Create new folder if missing
                if (!Directory.Exists(targetPath))
                {
                    try
                    {
                        Directory.CreateDirectory(targetPath);
                    }
                    catch (Exception ex)
                    {
                        Debug.LogWarning(ex);
                        Debug.LogError(string.Format("Cannot copy folder \'{0}\' to \'{1}\'", sourcePath, targetPath));
                        return;
                    }
                }

                for (int i = 0; i < folder.Children.Count; i++)
                {
                    var child = folder.Children[i];
                    var childExtension = Path.GetExtension(child.Path);
                    var childTargetPath = FileSystem.CombinePaths(targetPath, child.ShortName + childExtension);
                    Copy(folder.Children[i], childTargetPath);
                }
            }
            else
            {
                if (item.IsAsset || item.ItemType == ContentItemType.Scene)
                {
                    if (!AssetContent.CloneAssetFile(sourcePath, targetPath, Guid.NewGuid()))
                    {
                        Debug.LogError($"Cannot copy asset '{sourcePath}' to '{targetPath}'");
                    }
                }
                else
                {
                    // Copy file
                    try
                    {
                        File.Copy(sourcePath, targetPath, true);
                    }
                    catch (Exception ex)
                    {
                        Debug.LogWarning(ex);
                        Debug.LogError(string.Format("Cannot copy asset \'{0}\' to \'{1}\'", sourcePath, targetPath));
                        return;
                    }
                }
            }
        }

        public void Delete(ContentItem item, bool deletedByUser = false)
        {
            Debug.Assert(item != null, "Item cannot be null.");
            if (m_EnableEvents)
            {
                ItemRemoved?.Invoke(item);
            }

            item.OnDelete();
            m_ItemsDeleted++;
            string path = item.Path;

            if (item is ContentFolder folder)
            {
                // Delete all children
                if (folder.Children.Count > 0)
                {
                    var children = folder.Children.ToArray();
                    for (int i = 0; i < children.Length; i++)
                    {
                        Delete(children[i], deletedByUser);
                    }
                }

                if (deletedByUser && Directory.Exists(path))
                {
                    // FlaxEngine.Scripting.FlushRemovedObjects();

                    try
                    {
                        Directory.Delete(path, true);
                    }
                    catch (Exception ex)
                    {
                        Debug.LogWarning(ex);
                        Debug.LogWarning(string.Format("Cannot remove folder \'{0}\'", path));
                        return;
                    }
                }

                // Unlink from the parent
                item.ParentFolder = null;

                // Delete tree node
                folder.Node.Dispose();
            }
            else
            {
                // Delete asset file only if it was explicitly deleted by the user.
                // Some applications might modify the file by deleting the original and replacing
                // it with a new file which sometimes is caught in the middle of these operations.
                if (deletedByUser)
                {
                    // Check if it's an asset
                    if (item.IsAsset)
                    {
                        // Delete asset by using content pool
                        AssetContent.DeleteAsset(path);
                    }
/*                    else if (item is ScriptItem)
                    {
                        AssetContent.DeleteScript(path);
                    }*/
                    else
                    {
                        // Delete file
                        if (File.Exists(path))
                        {
                            File.Delete(path);
                        }
                    }
                }

                // Unlink from the parent
                item.ParentFolder = null;

                // Delete item
                item.Dispose();
            }

            if (m_EnableEvents)
            {
                WorkspaceModified?.Invoke();
            }
        }

        public void AddProxy(ContentOperate proxy, bool rebuild = false)
        {
            Operate.Insert(0, proxy);
            if (rebuild)
            {
                Rebuild();
            }
        }

        public void RemoveProxy(ContentOperate proxy, bool rebuild = false)
        {
            Operate.Remove(proxy);
            if (rebuild)
            {
                Rebuild();
            }
        }

        public void Rebuild(bool immediate = false)
        {
            m_RebuildFlag = true;
            if (immediate)
            {
                RebuildInternal();
            }
        }

        public override void OnInit()
        {
            AssetContent.AssetDisposing += OnContentAssetDisposing;
            Editor.Importing.ImportFileEnd += OnImportFileEnd;

            Operate.Add(new TextureOperate());
            Operate.Add(new ModelOperate());
            Operate.Add(new SceneOperate());

            // Create content folders nodes
            ProjectInfo project = Editor.GetProjectInfo();
            ProjectNode = new ProjectTreeNode(project);
            ProjectNode.Content = new MainContentTreeNode(ProjectNode, ContentFolderType.Content, EngineContext.ProjectContentFolder);
            ProjectNode.Content.GetFolder().ParentFolder = ProjectNode.GetFolder();
            Projects.Add(ProjectNode);

            RebuildInternal();
            m_EnableEvents = true;
        }

        /// <summary>
        /// Loads a project workspace and recursively loads its referenced projects.
        /// </summary>
        public void LoadProjects(ProjectInfo project)
        {
            ProjectTreeNode workspace = GetProjectWorkspace(project);
            if (workspace == null)
            {
                workspace = new ProjectTreeNode(project);
                Projects.Add(workspace);

                string contentPath = FileSystem.CombinePaths(project.ProjectFolderPath, "Content");
                if (Directory.Exists(contentPath))
                {
                    workspace.Content = new MainContentTreeNode(workspace, ContentFolderType.Content, contentPath);
                    workspace.Content.GetFolder().ParentFolder = workspace.GetFolder();
                }

                string sourcePath = FileSystem.CombinePaths(project.ProjectFolderPath, "Source");
                if (Directory.Exists(sourcePath))
                {
                    workspace.Source = new MainContentTreeNode(workspace, ContentFolderType.Source, sourcePath);
                    workspace.Source.GetFolder().ParentFolder = workspace.GetFolder();
                }
            }

            foreach (ProjectInfo.Reference reference in project.References)
            {
                LoadProjects(reference.Project);
            }
        }

        /// <summary>Called by a main content tree node's filesystem watcher.</summary>
        public void OnDirectoryEvent(MainContentTreeNode node, FileSystemEventArgs args)
        {
            if (m_IsDuringFastSetup)
            {
                return;
            }

            if (args.ChangeType is WatcherChangeTypes.Created or WatcherChangeTypes.Deleted or WatcherChangeTypes.Renamed or WatcherChangeTypes.Changed)
            {
                lock (m_DirtyNodesLock)
                {
                    m_DirtyNodes.Add(node);
                }
            }
        }

        public override void OnUpdate()
        {
            List<string> importedPaths = new();
            lock (m_ImportedPathsLock)
            {
                while (m_ImportedPaths.Count > 0)
                {
                    importedPaths.Add(m_ImportedPaths.Dequeue());
                }
            }
            foreach (string path in importedPaths)
            {
                ContentItem parent = Find(Path.GetDirectoryName(path));
                if (parent != null)
                {
                    RefreshFolder(parent, false);
                }
                OnImportFileDone(path);
            }

            MainContentTreeNode[] dirtyNodes;
            lock (m_DirtyNodesLock)
            {
                dirtyNodes = new MainContentTreeNode[m_DirtyNodes.Count];
                m_DirtyNodes.CopyTo(dirtyNodes);
                m_DirtyNodes.Clear();
            }

            foreach (MainContentTreeNode node in dirtyNodes)
            {
                LoadFolder(node, true);
                if (m_EnableEvents)
                {
                    WorkspaceModified?.Invoke();
                }
            }

            if (m_RebuildFlag)
            {
                RebuildInternal();
            }
        }

        public override void OnDispose()
        {
            AssetContent.AssetDisposing -= OnContentAssetDisposing;
            Editor.Importing.ImportFileEnd -= OnImportFileEnd;
            m_EnableEvents = false;
            Editor.Windows.Content?.DetachDatabaseTree();
            foreach (ContentOperate operate in Operate)
            {
                operate.Dispose();
            }
            foreach (ProjectTreeNode project in Projects)
            {
                project.Dispose();
            }
            Projects.Clear();
            ProjectNode = null;
            Operate.Clear();
        }

        private void OnContentAssetDisposing(Asset asset)
        {
            if (asset.ShouldDeleteFileOnUnload())
            {
                ContentItem item = Find(asset.ID);
                if (item != null)
                {
                    //Editor.Windows.CloseAllEditors(item);
                    item.Dispose();
                }
            }
        }

        private void OnImportFileEnd(IFileEntryAction entry, bool failed)
        {
            if (!failed && entry != null)
            {
                lock (m_ImportedPathsLock)
                {
                    m_ImportedPaths.Enqueue(entry.ResultUrl);
                }
            }
        }

        private static bool RenameAsset(ContentItem item, string newPath)
        {
            string oldPath = item.Path;
            if (item.IsAsset)
            {
                if (!AssetContent.RenameAsset(oldPath, newPath))
                {
                    Debug.LogError($"Cannot rename asset '{oldPath}' to '{newPath}'");
                    return false;
                }
            }
            else
            {
                // Rename file
                try
                {
                    File.Move(oldPath, newPath);
                }
                catch (Exception ex)
                {
                    Debug.LogWarning(ex);
                    Debug.LogError(string.Format("Cannot rename asset \'{0}\' to \'{1}\'", oldPath, newPath));
                    return false;
                }
            }
            item.UpdatePath(newPath);
            return true;
        }

        private static void UpdateAssetNewNameTree(ContentItem item)
        {
            string extension = Path.GetExtension(item.Path);

            string newPath = $"{item.ParentFolder.Path}/{item.ShortName + extension}";

            if (item.IsFolder)
            {
                string oldPath = item.Path;
                ContentFolder folder = (ContentFolder)item;

                // Create new folder
                try
                {
                    Directory.CreateDirectory(newPath);
                }
                catch (Exception ex)
                {
                    Debug.LogWarning(ex);
                    Debug.LogError(string.Format("Cannot move folder \'{0}\' to \'{1}\'", oldPath, newPath));
                    return;
                }

                item.UpdatePath(newPath);
                for (int i = 0; i < folder.Children.Count; i++)
                {
                    UpdateAssetNewNameTree(folder.Children[i]);
                }
            }
            else
            {
                RenameAsset(item, newPath);
            }
        }

        private void OnImportFileDone(string path)
        {
            if (Find(path) is BinaryAssetItem item)
            {
                AssetInfo info = default;
                if (AssetContent.GetAssetInfo(item.Path, ref info))
                {
                    Type assetType = Object.GetTypeFromTypeID(info.typeID);
                    if (item.TypeId != assetType)
                    {
                        ContentFolder toRefresh = item.ParentFolder;
                        OnAssetTypeInfoChanged(item, info, assetType);
                        RefreshFolder(toRefresh, false);
                    }
                    else
                    {
                        item.OnReimport(info.id);
                    }
                }
            }
        }

        private void OnAssetTypeInfoChanged(AssetItem item, AssetInfo info, Type assetType)
        {
            Debug.Logger.LogWarning("Asset", $"Asset '{item.Path}' changed type from {item.TypeId} to {info.typeID}");
            //Editor.Windows.CloseAllEditors(item);
            DisposeItem(item);
        }

        private void RebuildInternal()
        {
            bool enableEvents = m_EnableEvents;
            if (enableEvents)
            {
                WorkspaceRebuilding?.Invoke();
            }

            var stopwatch = System.Diagnostics.Stopwatch.StartNew();
            m_RebuildFlag = false;
            m_EnableEvents = false;
            m_IsDuringFastSetup = true;
            int startItems = m_ItemsCreated;
            foreach (ProjectTreeNode project in Projects)
            {
                if (project.Content != null)
                {
                    LoadFolder(project.Content, true);
                }
                if (project.Source != null)
                {
                    LoadFolder(project.Source, true);
                }
            }
            m_IsDuringFastSetup = false;
            m_EnableEvents = enableEvents;
            stopwatch.Stop();
            Debug.Logger.Log("Asset", $"Project database created in {(int)stopwatch.Elapsed.TotalMilliseconds} ms. Items count: {m_ItemsCreated - startItems}");

            if (enableEvents)
            {
                WorkspaceModified?.Invoke();
                WorkspaceRebuilt?.Invoke();
            }
        }

        private void DisposeItem(ContentItem item)
        {
            if (m_EnableEvents)
            {
                ItemRemoved?.Invoke(item);
            }

            if (item is ContentFolder folder)
            {
                while (folder.Children.Count > 0)
                {
                    DisposeItem(folder.Children[0]);
                }
                item.ParentFolder = null;
                folder.Node.Dispose();
            }
            else
            {
                item.ParentFolder = null;
                item.Dispose();
            }
        }

        private void LoadFolder(ContentTreeNode node, bool checkSubDirs)
        {
            if (node == null)
            {
                return;
            }

            ContentFolder folder = node.GetFolder();
            string path = folder.Path;
            bool canHaveAssets = node.GetCanHaveAssets();

            if (m_IsDuringFastSetup)
            {
                while (folder.Children.Count > 0)
                {
                    DisposeItem(folder.Children[0]);
                }
            }
            else
            {
                for (int i = 0; i < folder.Children.Count; i++)
                {
                    ContentItem child = folder.Children[i];
                    if (!child.Exists)
                    {
                        Debug.Logger.Log("Asset", $"Content item '{child.Path}' has been removed");
                        Delete(child, false);
                        i--;
                    }
                    else if (canHaveAssets && child is AssetItem assetItem)
                    {
                        AssetInfo info = default;
                        if (AssetContent.GetAssetInfo(child.Path, ref info))
                        {
                            bool changed = info.id != assetItem.Id;
                            Type assetType = Object.GetTypeFromTypeID(info.typeID);
                            if (!changed && assetType != assetItem.TypeId)
                            {
                                AssetOperate proxy = GetAssetOperate(assetItem.TypeId, child.Path);
                                if (proxy == null)
                                {
                                    proxy = GetAssetOperate(assetType, child.Path);
                                }
                                changed = proxy == null || !proxy.AcceptsAsset(assetType, child.Path);
                            }
                            if (changed)
                            {
                                OnAssetTypeInfoChanged(assetItem, info, assetType);
                                i--;
                            }
                        }
                    }
                }
            }

            string[] files = Directory.Exists(path)
                ? Directory.GetFiles(path, "*.*", SearchOption.TopDirectoryOnly)
                : Array.Empty<string>();
            if (canHaveAssets)
            {
                LoadAssets(node, files);
            }

            string[] childFolders = Directory.Exists(path) ? Directory.GetDirectories(path) : Array.Empty<string>();
            bool sortChildren = false;
            for (int i = 0; i < childFolders.Length; i++)
            {
                string normalized = FileSystem.NormalizePath(childFolders[i]);
                ContentFolder existing = m_IsDuringFastSetup ? null : folder.FindChild(normalized) as ContentFolder;
                if (existing == null)
                {
                    var childNode = new ContentTreeNode(node, normalized);
                    if (!m_IsDuringFastSetup)
                    {
                        sortChildren = true;
                    }
                    LoadFolder(childNode, true);
                    if (m_EnableEvents)
                    {
                        ItemAdded?.Invoke(childNode.GetFolder());
                        WorkspaceModified?.Invoke();
                    }
                    m_ItemsCreated++;
                }
                else if (checkSubDirs)
                {
                    LoadFolder(existing.Node, true);
                }
            }
            if (sortChildren)
            {
                node.SortChildren();
            }

            if (node is MainContentTreeNode && folder.ShortName == "Source")
            {
                HideChild(folder, $"{node.GetPath()}/obj");
                HideChild(folder, $"{node.GetPath()}/Properties");
            }
        }

        private void LoadAssets(ContentTreeNode parent, IEnumerable<string> files)
        {
            foreach (string filePath in files)
            {
                string path = FileSystem.NormalizePath(filePath);
                if (!m_IsDuringFastSetup && parent.GetFolder().ContainsChild(path))
                {
                    continue;
                }

                if (OperatingSystem.IsMacOS() && path.EndsWith(".DS_Store", StringComparison.Ordinal))
                {
                    continue;
                }

                AssetInfo info = default;
                ContentItem item = null;
                if (AssetContent.GetAssetInfo(path, ref info))
                {
                    Type assetType = Object.GetTypeFromTypeID(info.typeID);
                    AssetOperate proxy = GetAssetOperate(assetType, path);
                    if (proxy != null)
                    {
                        item = proxy.ConstructItem(path, assetType, info.id);
                    }
                }

                if (item == null)
                {
                    AssetOperate proxy = GetAssetVirtualOpreate(path);
                    if (proxy != null)
                    {
                        Type assetType = Object.GetTypeFromTypeID(info.typeID);
                        item = proxy.ConstructItem(path, assetType, info.id);
                    }

                    if (item == null)
                    {
                        item = new FileItem(path);
                    }
                }

                item.ParentFolder = parent.GetFolder();

                if (m_EnableEvents)
                {
                    ItemAdded?.Invoke(item);
                    WorkspaceModified?.Invoke();
                }

                m_ItemsCreated++;
            }
        }

        private static void HideChild(ContentFolder folder, string path)
        {
            if (folder.Find(FileSystem.NormalizePath(path)) is ContentFolder child)
            {
                child.Visible = false;
                child.Node.Visible = false;
            }
        }

    }
}
