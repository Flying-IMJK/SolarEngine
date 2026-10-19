using System;
using System.Collections.Generic;
using System.IO;
using SE.Editor.GUI;
using SE.GUI;

namespace SE.Editor
{
    /// <summary>
    /// Managed content browser backed by the editor asset database.
    /// </summary>
    public sealed class ContentWindow : EditorWindow
    {
        #region Window setup and lifecycle

        private RootContentTreeNode m_RootNode;
        private readonly SplitPanel m_Split;
        private readonly ToolStrip m_ToolStrip;
        private readonly ToolStripButton m_BackButton;
        private readonly ToolStripButton m_ForwardButton;
        private readonly ToolStripButton m_UpButton;
        private readonly NavigationBar m_NavigationBar;
        private readonly Panel m_ContentTreePanel;
        private readonly Tree m_Tree;
        private readonly SearchBox m_FoldersSearchBox;
        private readonly SearchBox m_ItemsSearchBox;
        private readonly ViewDropdown m_ViewDropdown;
        private readonly Panel m_ContentViewPanel;
        private readonly ContentView m_ContentView;
        private bool m_WorkspaceDirty;
        private string m_WorkspaceRebuildLocation = string.Empty;
        private SortType m_SortType = SortType.AlphabeticOrder;
        private bool m_ShowAllFiles = true;
        private bool m_ShowGeneratedFiles;
        private bool m_NavigationUnlocked;
        private readonly Stack<ContentTreeNode> m_NavigationUndo = new Stack<ContentTreeNode>(32);
        private readonly Stack<ContentTreeNode> m_NavigationRedo = new Stack<ContentTreeNode>(32);
        private ContentTreeNode m_SelectedNode;


        public ContentWindow(Editor editor) : base(editor, true)
        {
            Title = "Content";
            Icon = EditorIcons.Folder32;

            Editor.AssetDatabase.WorkspaceModified += OnWorkspaceModified;
            Editor.AssetDatabase.ItemRemoved += OnDatabaseItemRemoved;
            Editor.AssetDatabase.WorkspaceRebuilding += OnWorkspaceRebuilding;
            Editor.AssetDatabase.WorkspaceRebuilt += OnWorkspaceRebuilt;

            m_ToolStrip = AddChild(new ToolStrip(34.0f));
            m_ToolStrip.AddButton(EditorIcons.Import64, OnImportClicked);
            m_ToolStrip.AddSeparator();
            m_BackButton = m_ToolStrip.AddButton(EditorIcons.Left64, NavigateBackward);
            m_BackButton.TooltipText = "Navigate backward";
            m_ForwardButton = m_ToolStrip.AddButton(EditorIcons.Right64, NavigateForward);
            m_ForwardButton.TooltipText = "Navigate forward";
            m_UpButton = m_ToolStrip.AddButton(EditorIcons.Up64, NavigateUp);
            m_UpButton.TooltipText = "Navigate up";
            m_ToolStrip.AddSeparator();
            m_NavigationBar = m_ToolStrip.AddChild(new NavigationBar());

            m_Split = AddChild(new SplitPanel(Orientation.Horizontal, ScrollBars.None, ScrollBars.None));
            m_Split.SplitterValue = 0.2f;
            m_FoldersSearchBox = m_Split.Panel1.AddChild(new SearchBox(false, 4, 4));
            m_FoldersSearchBox.TextChanged += OnFoldersSearchChanged;
            m_ContentTreePanel = m_Split.Panel1.AddChild(new Panel(ScrollBars.Both));
            m_Tree = m_ContentTreePanel.AddChild(new Tree(false));
            m_Tree.SelectedChanged += OnTreeSelectionChanged;

            m_ViewDropdown = m_Split.Panel2.AddChild(new ViewDropdown());
            m_ViewDropdown.SupportMultiSelect = true;
            m_ViewDropdown.TooltipText = "Change content view and filter options";
            foreach (ContentItemSearchFilter filter in Enum.GetValues<ContentItemSearchFilter>())
            {
                m_ViewDropdown.Items.Add(filter.ToString());
            }
            m_ViewDropdown.SelectedIndexChanged += _ => UpdateItemsSearch();
            m_ViewDropdown.PopupCreate = CreateViewMenu;
            m_ItemsSearchBox = m_Split.Panel2.AddChild(new SearchBox(false, 0, 4));
            m_ItemsSearchBox.TextChanged += UpdateItemsSearch;
            m_ContentViewPanel = m_Split.Panel2.AddChild(new Panel(ScrollBars.Vertical));
            m_ContentView = m_ContentViewPanel.AddChild(new ContentView());
            m_ContentView.OnOpen += Open;
            m_ContentView.OnNavigateBack += NavigateBackward;
            m_ContentView.OnRename += Rename;
            m_ContentView.OnDelete += Delete;
            m_ContentView.OnDuplicate += Duplicate;
            m_ContentView.OnPaste += Paste;
        }

        /// <summary>
        /// Gets the selected tree node.
        /// </summary>
        public ContentTreeNode SelectedNode => m_SelectedNode;

        public ContentFolder CurrentViewFolder => m_SelectedNode?.GetFolder();

        public override void OnInit()
        {
            m_RootNode = new RootContentTreeNode
            {
                Name = "ContentWindow-Root",
                ChildrenIndent = 0,
            };
            m_RootNode.Expand(true);
            foreach (ProjectTreeNode project in Editor.AssetDatabase.Projects)
            {
                project.Parent = m_RootNode;
                project.Expand(true);
            }
            m_Tree.Margin = new Margin(0, 0, -16, 2);
            m_Tree.AddChild(m_RootNode);
            m_Tree.Name = "ContentWindow-Tree";
            m_NavigationUnlocked = true;
            m_Tree.Select(m_RootNode);
            m_SelectedNode = m_RootNode;
            NavigationClearHistory();
            m_WorkspaceDirty = true;
        }

        public override void OnUpdate()
        {
            if (m_WorkspaceDirty)
            {
                m_WorkspaceDirty = false;
                RefreshView();
            }
        }

        public override void OnExit()
        {
            Editor.AssetDatabase.WorkspaceModified -= OnWorkspaceModified;
            Editor.AssetDatabase.ItemRemoved -= OnDatabaseItemRemoved;
            Editor.AssetDatabase.WorkspaceRebuilding -= OnWorkspaceRebuilding;
            Editor.AssetDatabase.WorkspaceRebuilt -= OnWorkspaceRebuilt;
            m_ContentView.ClearItems();
            DetachDatabaseTree();
        }

        public void DetachDatabaseTree()
        {
            if (m_RootNode == null)
            {
                return;
            }
            m_NavigationUnlocked = false;
            m_ContentView.ClearItems();
            m_NavigationUndo.Clear();
            m_NavigationRedo.Clear();
            m_Tree.Select(m_RootNode);
            m_SelectedNode = m_RootNode;
            foreach (ProjectTreeNode project in Editor.AssetDatabase.Projects)
            {
                if (project.Parent == m_RootNode)
                {
                    project.Parent = null;
                }
            }
            m_NavigationUnlocked = true;
        }

        /// <summary>
        /// Navigates to the specified target content location.
        /// </summary>
        /// <param name="target">The target.</param>
        public void Navigate(ContentTreeNode target)
        {
            if (m_Tree != null)
            {
                m_Tree.Select(target ?? m_RootNode);
            }
        }

        private void Navigate(ContentTreeNode source, ContentTreeNode target)
        {
            if (target == null)
            {
                target = m_RootNode;
            }

            // Check if can do this action
            if (m_NavigationUnlocked && source != target)
            {
                // Lock navigation
                m_NavigationUnlocked = false;

                // Check if already added to the Undo on the top
                if (source != null && (m_NavigationUndo.Count == 0 || m_NavigationUndo.Peek() != source))
                {
                    // Add to Undo list
                    m_NavigationUndo.Push(source);
                }

                // Show folder contents and select tree node
                m_ContentView.IsSearching = false;
                RefreshView(target);
                m_SelectedNode = target;
                target.ExpandAllParents();

                // Clear redo list
                m_NavigationRedo.Clear();

                // Set valid sizes for stacks
                // RedoList.SetSize(32);
                // UndoList.SetSize(32);

                UpdateItemsSearch();

                // Unlock navigation
                m_NavigationUnlocked = true;

                UpdateUI();
            }
        }

        public void Refresh()
        {
            if (SelectedNode == m_RootNode)
            {
                Editor.AssetDatabase.Rebuild(true);
            }
            else if (CurrentViewFolder != null)
            {
                Editor.AssetDatabase.RefreshFolder(CurrentViewFolder, true);
            }
            RefreshView();
        }

        protected override void PerformLayoutBeforeChildren()
        {
            base.PerformLayoutBeforeChildren();

            if (m_Split == null || m_ToolStrip == null)
            {
                return;
            }
            m_ToolStrip.SetBounds(0, 0, Width, 34);
            m_NavigationBar.UpdateBounds(m_ToolStrip);
            m_Split.SetBounds(0, m_ToolStrip.Bottom, Width, MathF.Max(0, Height - m_ToolStrip.Bottom));
            m_FoldersSearchBox.SetBounds(4, 4, MathF.Max(0, m_Split.Panel1.Width - 8), 18);
            m_ContentTreePanel.SetBounds(0, 26, m_Split.Panel1.Width, MathF.Max(0, m_Split.Panel1.Height - 26));
            m_Tree.SetBounds(0, 0, m_ContentTreePanel.GetClientArea().Width, m_Tree.Height);
            m_ViewDropdown.SetBounds(4, 4, 50, 18);
            m_ItemsSearchBox.SetBounds(58, 4, MathF.Max(0, m_Split.Panel2.Width - 62), 18);
            m_ContentViewPanel.SetBounds(0, 30, m_Split.Panel2.Width, MathF.Max(0, m_Split.Panel2.Height - 30));
            m_ContentView.SetBounds(0, 0, m_ContentViewPanel.GetClientArea().Width, m_ContentView.Height);
        }

        #endregion

        #region Browser and navigation

        public void RefreshView()
        {
            if (m_ContentView.IsSearching)
            {
                UpdateItemsSearch();
            }
            else
            {
                RefreshView(SelectedNode);
            }
        }

        public void RefreshView(ContentTreeNode target)
        {
            if (target == null)
            {
                return;
            }

            m_ContentView.IsSearching = false;
            List<ContentItem> items = new();
            if (target == m_RootNode)
            {
                foreach (Control child in m_RootNode.Children)
                {
                    if (child is ContentTreeNode node)
                    {
                        items.Add(node.GetFolder());
                    }
                }
            }
            else
            {
                foreach (ContentItem item in target.GetFolder().Children)
                {
                    if (IsVisibleItem(item))
                    {
                        items.Add(item);
                    }
                }
            }
            m_ContentView.ShowItems(items, m_SortType, keepSelection: true);
        }

        private bool IsVisibleItem(ContentItem item)
        {
            if (!m_ShowAllFiles && item is FileItem)
            {
                return false;
            }
            if (!m_ShowGeneratedFiles &&
                (item.Path.EndsWith(".Gen.cs", StringComparison.OrdinalIgnoreCase) ||
                 item.Path.EndsWith(".Gen.h", StringComparison.OrdinalIgnoreCase) ||
                 item.Path.EndsWith(".Gen.cpp", StringComparison.OrdinalIgnoreCase) ||
                 item.Path.EndsWith(".csproj", StringComparison.OrdinalIgnoreCase) ||
                 item.Path.Contains(".CSharp", StringComparison.OrdinalIgnoreCase)))
            {
                return false;
            }
            return true;
        }

        private void OnWorkspaceModified()
        {
            m_WorkspaceDirty = true;
        }

        private void OnWorkspaceRebuilding()
        {
            m_WorkspaceRebuildLocation = SelectedNode?.GetPath() ?? string.Empty;
        }

        private void OnWorkspaceRebuilt()
        {
            NavigationClearHistory();
            ContentFolder folder = Editor.AssetDatabase.Find(m_WorkspaceRebuildLocation) as ContentFolder;
            if (folder?.Node != null)
            {
                Navigate(folder.Node);
            }
            else
            {
                ShowRoot();
            }
            UpdateItemsSearch();
        }

        private void OnDatabaseItemRemoved(ContentItem item)
        {
            if (item is not ContentFolder folder)
            {
                return;
            }
            if (CurrentViewFolder != null && folder.Find(CurrentViewFolder))
            {
                ShowRoot();
            }
            if (m_NavigationUndo.Contains(folder.Node) || m_NavigationRedo.Contains(folder.Node))
            {
                NavigationClearHistory();
            }
        }

        private void OnTreeSelectionChanged(List<TreeNode> from, List<TreeNode> to)
        {
            ContentTreeNode source = null;
            if (from.Count > 0)
            {
                source = from[0] as ContentTreeNode;
            }

            ContentTreeNode target = null;
            if (to.Count > 0)
            {
                target = to[0] as ContentTreeNode;
            }

            Navigate(source, target);
        }

        public void NavigateBackward()
        {
            if (!m_NavigationUnlocked || m_NavigationUndo.Count == 0)
            {
                return;
            }
            ContentTreeNode target = m_NavigationUndo.Pop();
            m_NavigationRedo.Push(SelectedNode);
            m_NavigationUnlocked = false;
            m_Tree.Select(target);
            m_NavigationUnlocked = true;
            m_SelectedNode = target;
            RefreshView(target);
            UpdateItemsSearch();
            UpdateUI();
            m_ContentView.SelectFirstItem();
        }

        public void NavigateForward()
        {
            if (!m_NavigationUnlocked || m_NavigationRedo.Count == 0)
            {
                return;
            }
            ContentTreeNode target = m_NavigationRedo.Pop();
            m_NavigationUndo.Push(SelectedNode);
            m_NavigationUnlocked = false;
            m_Tree.Select(target);
            m_NavigationUnlocked = true;
            m_SelectedNode = target;
            RefreshView(target);
            UpdateItemsSearch();
            UpdateUI();
            m_ContentView.SelectFirstItem();
        }

        public void NavigateUp()
        {
            Navigate(SelectedNode?.ParentNode() ?? m_RootNode);
        }

        public void NavigationClearHistory()
        {
            m_NavigationUndo.Clear();
            m_NavigationRedo.Clear();
            UpdateUI();
        }

        public void ShowRoot()
        {
            if (m_RootNode != null)
            {
                Navigate(m_RootNode);
            }
        }

        public void Select(Asset asset)
        {
            if (asset != null)
            {
                Select(Editor.AssetDatabase.Find(asset.ID));
            }
        }

        public void Select(ContentItem item, bool fastScroll = false)
        {
            if (item?.ParentFolder == null || !item.ParentFolder.Visible || !m_NavigationUnlocked)
            {
                return;
            }
            FocusOrShow();
            Navigate(item.ParentFolder.Node);
            m_ContentView.Select(item);
            m_ContentViewPanel.ScrollViewTo(item, fastScroll);
            m_ContentView.Focus();
        }

        public void Open(ContentItem item)
        {
            if (item is ContentFolder folder)
            {
                folder.Node.Expand(true);
                Navigate(folder.Node);
                m_ContentView.SelectFirstItem();
                return;
            }
            if (item != null)
            {
                Editor.AssetDatabase.GetProxy(item)?.Open(item);
            }
        }

        public void ClearItemsSearch()
        {
            m_ItemsSearchBox.Text = string.Empty;
        }

        private void OnFoldersSearchChanged()
        {
            if (m_RootNode == null)
            {
                return;
            }
            m_RootNode.LockChildrenRecursive();
            m_RootNode.UpdateFilter(m_FoldersSearchBox.Text);
            m_RootNode.UnlockChildrenRecursive();
            m_Tree.PerformLayout();
        }

        private void UpdateItemsSearch()
        {
            if (m_RootNode == null || SelectedNode == null)
            {
                return;
            }
            string query = m_ItemsSearchBox.Text.Trim();
            if (query.Length == 0 && !m_ViewDropdown.HasSelection)
            {
                RefreshView(SelectedNode);
                return;
            }

            List<ContentItem> items = new();
            if (Guid.TryParse(query, out Guid id))
            {
                AssetItem asset = Editor.AssetDatabase.FindAsset(id);
                if (asset != null)
                {
                    items.Add(asset);
                }
            }
            else if (SelectedNode == m_RootNode)
            {
                foreach (Control child in m_RootNode.Children)
                {
                    if (child is ContentTreeNode node)
                    {
                        CollectSearchItems(node.GetFolder(), items, query);
                    }
                }
            }
            else
            {
                CollectSearchItems(CurrentViewFolder, items, query);
            }
            m_ContentView.IsSearching = true;
            m_ContentView.ShowItems(items, m_SortType);
        }

        private void CollectSearchItems(ContentFolder folder, List<ContentItem> items, string query)
        {
            foreach (ContentItem child in folder.Children)
            {
                if (child is ContentFolder subfolder)
                {
                    CollectSearchItems(subfolder, items, query);
                }
                else if (IsVisibleItem(child) &&
                         (!m_ViewDropdown.HasSelection || m_ViewDropdown.Selection.Contains((int)child.SearchFilter)) &&
                         (query.Length == 0 || child.ShortName.Contains(query, StringComparison.OrdinalIgnoreCase)))
                {
                    items.Add(child);
                }
            }
        }

        private void UpdateUI()
        {
            m_BackButton.Enabled = m_NavigationUndo.Count > 0;
            m_ForwardButton.Enabled = m_NavigationRedo.Count > 0;
            m_UpButton.Enabled = SelectedNode != null && SelectedNode != m_RootNode;
            UpdateNavigationBar();
        }

        private void UpdateNavigationBar()
        {
            if (m_NavigationBar == null)
            {
                return;
            }
            m_NavigationBar.DisposeChildren();
            List<ContentTreeNode> nodes = new();
            for (ContentTreeNode node = SelectedNode; node != null; node = node.ParentNode())
            {
                nodes.Add(node);
            }
            float x = NavigationBar.DefaultButtonsMargin;
            float height = m_ToolStrip.ItemsHeight - 2 * ToolStrip.DefaultMarginV;
            for (int i = nodes.Count - 1; i >= 0; i--)
            {
                ContentNavigationButton button = new(nodes[i], x, ToolStrip.DefaultMarginV, height);
                button.PerformLayout();
                m_NavigationBar.AddChild(button);
                x += button.Width + NavigationBar.DefaultButtonsMargin;
                if (i > 0)
                {
                    ContentNavigationSeparator separator = new(button, x, ToolStrip.DefaultMarginV, height);
                    m_NavigationBar.AddChild(separator);
                    x += separator.Width + NavigationBar.DefaultButtonsMargin;
                }
            }
            m_NavigationBar.PerformLayout();
        }

        private void OnImportClicked()
        {
            if (CurrentViewFolder?.CanHaveAssets == true)
            {
                Editor.Importing.ShowImportFileDialog(CurrentViewFolder);
            }
        }

        private ContextMenu CreateViewMenu(ComboBox comboBox)
        {
            ContextMenu menu = new();
            ContextMenuButton viewScale = menu.AddButton("View Scale");
            viewScale.CloseMenuOnClick = false;
            FloatValueBox scaleValue = new(m_ContentView.ViewScale, 75, 2, 50, 0.3f, 3.0f, 0.01f);
            scaleValue.Parent = viewScale;
            scaleValue.ValueChanged += () => m_ContentView.ViewScale = scaleValue.GetValue();
            ContextMenuChildMenu viewType = menu.AddChildMenu("View Type");
            viewType.ContextMenu.AddButton("Tiles", () => m_ContentView.ViewType = ContentViewType.Tiles)
                .SetChecked(m_ContentView.ViewType == ContentViewType.Tiles);
            viewType.ContextMenu.AddButton("List", () => m_ContentView.ViewType = ContentViewType.List)
                .SetChecked(m_ContentView.ViewType == ContentViewType.List);

            ContextMenuChildMenu show = menu.AddChildMenu("Show");
            show.ContextMenu.AddButton("File extensions", () => m_ContentView.ShowFileExtensions = !m_ContentView
                .ShowFileExtensions)
                .SetChecked(m_ContentView.ShowFileExtensions);
            show.ContextMenu.AddButton("Generated files", () =>
            {
                m_ShowGeneratedFiles =
                !m_ShowGeneratedFiles; RefreshView();
            })
                .SetChecked(m_ShowGeneratedFiles);
            show.ContextMenu.AddButton("All files", () => { m_ShowAllFiles = !m_ShowAllFiles; RefreshView(); })
                .SetChecked(m_ShowAllFiles);

            ContextMenuChildMenu filters = menu.AddChildMenu("Filters");
            for (int i = 0; i < m_ViewDropdown.Items.Count; i++)
            {
                int index = i;
                filters.ContextMenu.AddButton(m_ViewDropdown.Items[i], () => m_ViewDropdown.OnClicked(index));
            }
            ContextMenuChildMenu sort = menu.AddChildMenu("Sort by");
            sort.ContextMenu.AddButton("Alphabetic Order", () =>
            {
                m_SortType = SortType
                .AlphabeticOrder; RefreshView();
            })
                .SetChecked(m_SortType == SortType.AlphabeticOrder);
            sort.ContextMenu.AddButton("Alphabetic Reverse", () =>
            {
                m_SortType = SortType
                .AlphabeticReverse; RefreshView();
            })
                .SetChecked(m_SortType == SortType.AlphabeticReverse);
            return menu;
        }

        #endregion

        #region Content operations

        private NewItem m_NewElement;
        private Action<ContentItem> m_NewItemCreated;

        public void Rename(ContentItem item)
        {
            if (item == null || !item.CanRename)
            {
                return;
            }
            Select(item, true);
            SetContentScrolling(false);
            RenamePopup popup = RenamePopup.ShowPopup(item, item.TextRectangle, item.ShortName, false);
            popup.Validate = ValidateRename;
            popup.Renamed += p => Rename(item, p.Text);
            popup.Closed += _ =>
            {
                SetContentScrolling(true);
                if (m_NewElement != null)
                {
                    DisposeNewItem();
                }
            };
            if (item == m_NewElement)
            {
                popup.InitialValue = "?";
            }
        }

        public void Rename(ContentItem item, string newShortName)
        {
            if (item == null || !item.CanRename || !IsValidName(newShortName) || item.ParentFolder == null)
            {
                return;
            }
            newShortName = newShortName.TrimEnd();
            string extension = item.IsFolder ? string.Empty : Path.GetExtension(item.Path);
            if (!item.IsFolder && newShortName.EndsWith(extension, StringComparison.OrdinalIgnoreCase))
            {
                newShortName = newShortName.Substring(0, newShortName.Length - extension.Length);
            }
            ContentFolder parent = item.ParentFolder;
            string newPath = FileSystem.NormalizePath(Path.Combine(parent.Path, newShortName + extension));
            if (item == m_NewElement)
            {
                item.ParentFolder = null;
                item.Dispose();
                m_NewElement = null;
                item = null;
                ContentOperate proxy = m_NewItemProxy;
                proxy.Create(newPath, m_NewItemArgument);
            }
            else
            {
                Editor.AssetDatabase.Move(item, newPath);
            }
            Editor.AssetDatabase.RefreshFolder(parent, false);
            RefreshView();
            ContentItem created = parent.FindChild(newPath);
            if (created != null)
            {
                Select(created, true);
                m_NewItemCreated?.Invoke(created);
            }
            m_NewItemCreated = null;
            m_NewItemProxy = null;
            m_NewItemArgument = null;
        }

        private ContentOperate m_NewItemProxy;
        private object m_NewItemArgument;

        private static bool ValidateRename(RenamePopup popup, ref string value)
        {
            return IsValidName(value);
        }

        private static bool IsValidName(string value)
        {
            return !string.IsNullOrWhiteSpace(value) &&
                   value != "." && value != ".." &&
                   value.IndexOfAny(Path.GetInvalidFileNameChars()) < 0;
        }

        private void SetContentScrolling(bool enabled)
        {
            if (m_ContentViewPanel.vScrollBar != null)
            {
                m_ContentViewPanel.vScrollBar.ThumbEnabled = enabled;
            }
            if (m_ContentViewPanel.hScrollBar != null)
            {
                m_ContentViewPanel.hScrollBar.ThumbEnabled = enabled;
            }
        }

        private void DisposeNewItem()
        {
            m_NewElement.ParentFolder = null;
            m_NewElement.Dispose();
            m_NewElement = null;
            m_NewItemCreated = null;
            m_NewItemProxy = null;
            m_NewItemArgument = null;
            RefreshView();
        }

        public void Delete(ContentItem item)
        {
            List<ContentItem> selected = m_ContentView.GetSelection();
            Delete(item != null && selected.Contains(item) ? new List<ContentItem>(selected)
                : new List<ContentItem> { item });
        }

        public void Delete(List<ContentItem> items)
        {
            if (items == null || items.Count == 0)
            {
                return;
            }
            string message = items.Count == 1
                ? $"Are you sure to delete '{items[0].Path}'?\nThis action cannot be undone. Files will be deleted permanently."
                : $"Are you sure to delete {items.Count} selected items?\nThis action cannot be undone. Files will be deleted permanently.";
            if (MessageBox.Show(message, "Delete asset(s)", MessageBoxButtons.OKCancel, MessageBoxIcon.Question)
                != DialogResult.OK)
            {
                return;
            }
            List<ContentItem> toDelete = new(items);
            toDelete.Sort((left, right) => left.IsFolder.CompareTo(right.IsFolder));
            NavigationClearHistory();
            foreach (ContentItem item in toDelete)
            {
                Editor.AssetDatabase.Delete(item, true);
            }
            RefreshView();
        }

        public void Duplicate(ContentItem item)
        {
            if (item?.ParentFolder == null)
            {
                return;
            }
            string path = GetClonedAssetPath(item);
            Editor.AssetDatabase.Copy(item, path);
            Editor.AssetDatabase.RefreshFolder(item.ParentFolder, true);
            RefreshView();
            ContentItem clone = item.ParentFolder.FindChild(path);
            if (clone != null)
            {
                Select(clone);
                Rename(clone);
            }
        }

        public void Duplicate(List<ContentItem> items)
        {
            if (items == null || items.Count == 0)
            {
                return;
            }
            if (items.Count == 1)
            {
                Duplicate(items[0]);
                return;
            }
            foreach (ContentItem item in new List<ContentItem>(items))
            {
                if (item.ParentFolder != null)
                {
                    Editor.AssetDatabase.Copy(item, GetClonedAssetPath(item));
                }
            }
            Editor.AssetDatabase.RefreshFolder(CurrentViewFolder, true);
            RefreshView();
        }

        public void Paste(List<string> files)
        {
            if (files == null || CurrentViewFolder == null)
            {
                return;
            }
            List<string> importFiles = new();
            foreach (string sourcePath in files)
            {
                ContentItem item = Editor.AssetDatabase.Find(sourcePath);
                if (item == null)
                {
                    importFiles.Add(sourcePath);
                    continue;
                }
                string targetPath = FileSystem.NormalizePath(Path.Combine(CurrentViewFolder.Path, item.FileName));
                if (string.Equals(sourcePath, targetPath, StringComparison.OrdinalIgnoreCase))
                {
                    targetPath = GetClonedAssetPath(item);
                }
                Editor.AssetDatabase.Copy(item, targetPath);
            }
            if (importFiles.Count > 0)
            {
                Editor.Importing.Import(importFiles, CurrentViewFolder);
            }
            Editor.AssetDatabase.RefreshFolder(CurrentViewFolder, false);
            RefreshView();
        }

        public void CreateItem(ContentOperate proxy, Action<ContentItem> created = null,
            string initialName = null, bool withRenaming = false, object argument = null)
        {
            if (proxy == null)
            {
                return;
            }
            if (CurrentViewFolder == null || !proxy.CanCreate(CurrentViewFolder))
            {
                Navigate(Editor.AssetDatabase.ProjectNode?.Content);
            }
            ContentFolder parent = CurrentViewFolder;
            if (parent == null || !proxy.CanCreate(parent))
            {
                return;
            }
            string name = string.IsNullOrEmpty(initialName) || !proxy.IsFileNameValid(initialName)
                ? proxy.NewItemName : initialName;
            string extension = "." + proxy.FileExtension.TrimStart('.');
            string path = FileSystem.NormalizePath(Path.Combine(parent.Path, name + extension));
            for (int index = 0; parent.FindChild(path) != null || File.Exists(path); index++)
            {
                path = FileSystem.NormalizePath(Path.Combine(parent.Path, $"{name} {index}{extension}"));
            }
            if (withRenaming)
            {
                m_NewItemCreated = created;
                m_NewItemProxy = proxy;
                m_NewItemArgument = argument;
                m_NewElement = new NewItem(path, proxy, IntPtr.Zero);
                m_NewElement.ParentFolder = parent;
                RefreshView();
                Rename(m_NewElement);
                return;
            }
            proxy.Create(path, argument);
            Editor.AssetDatabase.RefreshFolder(parent, false);
            RefreshView();
            ContentItem newItem = parent.FindChild(path);
            if (newItem != null)
            {
                Select(newItem, true);
                created?.Invoke(newItem);
            }
        }

        private static string GetClonedAssetPath(ContentItem item)
        {
            string parent = Path.GetDirectoryName(item.Path);
            string extension = item.IsFolder ? string.Empty : Path.GetExtension(item.Path);
            for (int index = 1; ; index++)
            {
                string name = index == 1 ? item.ShortName + " Copy" : item.ShortName + " Copy " + index;
                string result = FileSystem.NormalizePath(Path.Combine(parent, name + extension));
                if (!File.Exists(result) && !Directory.Exists(result))
                {
                    return result;
                }
            }
        }

        #endregion

        #region Context menu

        public override bool OnMouseDown(Float2 location, MouseButton button)
        {
            if (button == MouseButton.Extended1)
            {
                NavigateBackward();
            }
            else if (button == MouseButton.Extended2)
            {
                NavigateForward();
            }
            return base.OnMouseDown(location, button);
        }

        public override bool OnMouseUp(Float2 location, MouseButton button)
        {
            Control control = GetChildAtRecursive(location);
            if (button == MouseButton.Right)
            {
                if (control is ContentItem item)
                {
                    if (!m_ContentView.IsSelected(item))
                    {
                        m_ContentView.Select(item);
                    }
                    ShowContextMenuForItem(item, location, false);
                }
                else if (control is ContentView)
                {
                    ShowContextMenuForItem(null, location, false);
                }
                else if (control is ContentTreeNode node)
                {
                    m_Tree.Select(node);
                    ShowContextMenuForItem(node.GetFolder(), location, true);
                }
                return true;
            }
            if (button == MouseButton.Left && control is ContentView)
            {
                m_ContentView.ClearSelection();
                return true;
            }
            return base.OnMouseUp(location, button);
        }

        private void ShowContextMenuForItem(ContentItem item, Float2 location, bool isTreeNode)
        {
            bool isValidElement = item != null;
            ContentOperate proxy = Editor.AssetDatabase.GetProxy(item);
            bool isFolder = isValidElement && item.IsFolder;
            ContentFolder folder = isValidElement
                ? isFolder ? (ContentFolder)item : item.ParentFolder
                : CurrentViewFolder;
            bool isRootFolder = CurrentViewFolder == m_RootNode.GetFolder();

            ContextMenu menu = new ContextMenu { Tag = item };
            if (isTreeNode)
            {
                menu.AddButton("Expand All", () => CurrentViewFolder.Node.ExpandAll()).Enabled =
                    CurrentViewFolder.Node.ChildrenCount != 0;
                menu.AddButton("Collapse All", () => CurrentViewFolder.Node.CollapseAll()).Enabled =
                    CurrentViewFolder.Node.ChildrenCount != 0;
                menu.AddSeparator();
            }

            if (item is ContentFolder { Node: ProjectTreeNode })
            {
                menu.AddButton("Show in explorer", () => FileSystem.ShowFileExplorer(CurrentViewFolder.Path));
            }
            else if (isValidElement)
            {
                menu.AddButton("Open", () => Open(item)).Enabled = proxy != null || isFolder;
                if (m_ContentView.SelectedCount > 1)
                {
                    menu.AddButton("Open (all selected)", () =>
                    {
                        foreach (ContentItem selectedItem in m_ContentView.GetSelection())
                        {
                            Open(selectedItem);
                        }
                    });
                }

                menu.AddButton("Show in explorer", () => FileSystem.ShowFileExplorer(Path.GetDirectoryName(item.Path)));

                if (!item.HasDefaultThumbnail)
                {
                    if (m_ContentView.SelectedCount > 1)
                    {
                        menu.AddButton("Refresh thumbnails", () =>
                        {
                            foreach (ContentItem selectedItem in m_ContentView.GetSelection())
                            {
                                selectedItem.RefreshThumbnail();
                            }
                        });
                    }
                    else
                    {
                        menu.AddButton("Refresh thumbnail", item.RefreshThumbnail);
                    }
                }

                if (!isFolder)
                {
                    if (item is BinaryAssetItem binaryAsset &&
                        !binaryAsset.GetImportPath(out string importPath))
                    {
                        string importLocation = Path.GetDirectoryName(importPath);
                        if (!string.IsNullOrEmpty(importLocation) && Directory.Exists(importLocation))
                        {
                            menu.AddButton("Show import location", () =>
                                FileSystem.ShowFileExplorer(importLocation));
                        }
                    }

                    if (item is AssetItem assetItem && assetItem.GetIsLoaded())
                    {
                        menu.AddButton("Reload", assetItem.Reload);
                    }
                }

                if (isFolder && folder.Node is MainContentTreeNode)
                {
                    menu.AddSeparator();
                }
                else
                {
                    menu.AddButton("Delete", () => Delete(item));
                }

                menu.AddButton("Paste", () => m_ContentView.Paste()).Enabled = m_ContentView.CanPaste();
                if (!(isFolder && folder.Node is MainContentTreeNode))
                {
                    menu.AddButton("Rename", () => Rename(item));
                }
            }
            else
            {
                menu.AddButton("Show in explorer", () => FileSystem.ShowFileExplorer(CurrentViewFolder.Path));
                menu.AddButton("Paste", () => m_ContentView.Paste()).Enabled = m_ContentView.CanPaste();
                menu.AddButton("Refresh", () => Editor.AssetDatabase.RefreshFolder(CurrentViewFolder, true));
                menu.AddButton("Refresh all thumbnails", m_ContentView.RefreshThumbnails);
            }

            menu.AddSeparator();

            if (!isRootFolder && item is not ContentFolder { Node: ProjectTreeNode })
            {
                menu.AddButton("New folder", () => { });
            }

            if (!isRootFolder)
            {
                ContextMenuChildMenu createMenu = menu.GetOrAddChildMenu("Create");
                foreach (ContentOperate operate in Editor.AssetDatabase.Operate)
                {
                    if (operate.IsProxyFor(typeof(SceneAsset)))
                    {
                        createMenu.ContextMenu.AddButton("Scene", () => CreateItem(operate));
                        break;
                    }
                }
            }

            if (folder.CanHaveAssets)
            {
                menu.AddButton("Import file", () => { });
            }

            menu.Show(this, location);
        }

        private void CreateFolder(ContentFolder parent)
        {
            if (parent == null || parent.Node is RootContentTreeNode or ProjectTreeNode)
            {
                return;
            }
            string name = "New Folder";
            string path = FileSystem.NormalizePath(Path.Combine(parent.Path, name));
            for (int index = 1; Directory.Exists(path) || File.Exists(path); index++)
            {
                path = FileSystem.NormalizePath(Path.Combine(parent.Path, name + " " + index));
            }

            Directory.CreateDirectory(path);
            Editor.AssetDatabase.RefreshFolder(parent, false);
            ContentItem created = parent.FindChild(path);
            RefreshView();
            if (created != null)
            {
                Select(created, true);
                Rename(created);
            }
        }

        #endregion

        #region View dropdown

        public class ViewDropdown : ComboBox
        {
            public void OnClicked(int index)
            {
                OnItemClicked(index);
            }

            /// <inheritdoc />
            public override void Draw()
            {
                // Cache data
                Rectangle clientRect = new Rectangle(Float2.Zero, Size);
                float margin = clientRect.Height * 0.2f;
                float boxSize = clientRect.Height - margin * 2;
                bool isOpened = IsPopupOpened;
                bool enabled = EnabledInHierarchy;
                Color backgroundColor = BackgroundColor;
                Color borderColor = BorderColor;
                Color arrowColor = ArrowColor;
                if (!enabled)
                {
                    backgroundColor *= 0.5f;
                    arrowColor *= 0.7f;
                }
                else if (isOpened || _mouseDown)
                {
                    backgroundColor = BackgroundColorSelected;
                    borderColor = BorderColorSelected;
                    arrowColor = ArrowColorSelected;
                }
                else if (IsMouseOver)
                {
                    backgroundColor = BackgroundColorHighlighted;
                    borderColor = BorderColorHighlighted;
                    arrowColor = ArrowColorHighlighted;
                }

                // Background
                Render2D.FillRectangle(clientRect, backgroundColor);
                Render2D.DrawRectangle(clientRect, borderColor);

                // Draw text
                float textScale = Height / DefaultHeight;
                Rectangle textRect = new Rectangle(margin, 0, clientRect.Width - boxSize - 2.0f * margin, clientRect
                    .Height);
                Render2D.PushClip(textRect);
                Color textColor = TextColor;
                Render2D.RenderText(Font.GetFont(), "View", textRect, enabled ? textColor : textColor * 0.5f,
                    TextAlignment.Near, TextAlignment.Center, TextWrapping.NoWrap, 1.0f, textScale);
                Render2D.PopClip();

                // Arrow
                if (ArrowImage != null)
                {
                    ArrowImage.Draw(new Rectangle(clientRect.Width - margin - boxSize, margin, boxSize, boxSize),
                        arrowColor);
                }
            }

            /// <inheritdoc />
            public override bool OnMouseUp(Float2 location, MouseButton button)
            {
                // Check flags
                if (_mouseDown && !_blockPopup)
                {
                    // Clear flag
                    _mouseDown = false;

                    // Ensure to have valid menu
                    if (_popupMenu == null)
                    {
                        _popupMenu = OnCreatePopup();
                        _popupMenu.MaximumItemsInViewCount = MaximumItemsInViewCount;
                        _popupMenu.VisibleChanged += cm =>
                        {
                            RootControl win = Root;
                            _blockPopup = win != null && new Rectangle(Float2.Zero, Size)
                                .Contains(PointFromWindow(win.GetMousePosition()));
                            if (!_blockPopup)
                            {
                                Focus();
                            }
                        };
                    }

                    // Check if menu hs been already shown
                    if (_popupMenu.Visible)
                    {
                        // Hide
                        _popupMenu.Hide();
                        return true;
                    }

                    // Show
                    _popupMenu.Show(this, new Float2(1, Height));
                }
                else
                {
                    _blockPopup = false;
                }

                return true;
            }
        }

        #endregion
    }
}
