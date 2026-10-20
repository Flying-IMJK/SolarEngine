using System;
using System.Collections.Generic;
using SE.Editor.SceneGraph;
using SE.GUI;

namespace SE.Editor.GUI
{
    public sealed class SceneHierarchyWindow : SceneEditorWindow
    {
        private readonly SearchBox m_SearchBox;
        private readonly Tree m_Tree;
        private readonly Panel m_SceneTreePanel;
        private readonly List<(string Path, Type ActorType)> m_ActorCreateMenus = new();
        private bool m_IsUpdatingSelection;
        private bool m_IsMouseDown;

        public Panel SceneTreePanel => m_SceneTreePanel;

        public SceneHierarchyWindow(Editor editor) : base(editor)
        {
            Title = "Hierarchy";
            Icon = EditorIcons.HierarchyWindow32;

            ContainerControl headerPanel = AddChild(new ContainerControl());
            headerPanel.AnchorPreset = AnchorPresets.HorizontalStretchTop;
            headerPanel.BackgroundColor = Style.Current.Background;
            headerPanel.IsScrollable = false;
            headerPanel.Offsets = new Margin(0, 0, 0, 18 + 6);

            m_SearchBox = headerPanel.AddChild(new SearchBox());
            m_SearchBox.AnchorPreset = AnchorPresets.HorizontalStretchMiddle;
            m_SearchBox.Bounds = new Rectangle(4, 4, headerPanel.Width - 8, 18);
            m_SearchBox.TextChanged += OnSearchBoxTextChanged;
            m_SearchBox.Name = "SceneHierarchyWindow-SearchBox";

            m_SceneTreePanel = AddChild(new Panel());
            m_SceneTreePanel.AnchorPreset = AnchorPresets.StretchAll;
            m_SceneTreePanel.Offsets = new Margin(0, 0, headerPanel.Bottom, 0);
            m_SceneTreePanel.IsScrollable = true;
            m_SceneTreePanel.SetScrollBars(ScrollBars.Both);
            m_SceneTreePanel.Name = "SceneHierarchyWindow-SceneTreePanel";

            ActorTreeNode root = editor.Scene.Root.TreeNode;
            root.ChildrenIndent = 0;
            root.Expand();
            root.Name = "SceneHierarchyWindow-Tree/Root";

            m_Tree = m_SceneTreePanel.AddChild(new Tree(true));
            m_Tree.Margin = new Margin(0.0f, 0.0f, -16.0f, m_SceneTreePanel.GetScrollBarsSize());
            m_Tree.IsScrollable = true;
            m_Tree.AddChild(root);
            m_Tree.SelectedChanged += TreeOnSelectedChanged;
            m_Tree.RightClick += OnTreeRightClick;
            m_Tree.Name = "SceneHierarchyWindow-Tree";
        }

        public void ScrollingOnSceneTreeView(bool enabled)
        {
            if (m_SceneTreePanel.vScrollBar != null)
            {
                m_SceneTreePanel.vScrollBar.ThumbEnabled = enabled;
            }
            if (m_SceneTreePanel.hScrollBar != null)
            {
                m_SceneTreePanel.hScrollBar.ThumbEnabled = enabled;
            }
        }

        public void Search()
        {
            m_SearchBox.Focus();
        }

        public void ShowContextMenu(Control parent, Float2 location)
        {
            CreateContextMenu().Show(parent, location);
        }

        public override void OnInit()
        {
            m_ActorCreateMenus.Add(("Camera", typeof(SE.Camera)));
            m_ActorCreateMenus.Add(("Light/Direction", typeof(SE.DirectionalLight)));
            m_ActorCreateMenus.Add(("Light/Point", typeof(SE.PointLight)));
            m_ActorCreateMenus.Add(("Model/Static", typeof(SE.StaticModel)));
        }

        public override void Draw()
        {
            Style style = Style.Current;
            string overlayText = string.Empty;
            TextWrapping textWrap = TextWrapping.NoWrap;
            if (m_Tree.GetChild(0) is ContainerControl root && root.ChildrenCount == 0)
            {
                overlayText = "No scene\nOpen one from the content window";
                textWrap = TextWrapping.WrapWords;
            }

            if (overlayText.Length != 0)
            {
                Render2D.RenderText(style.FontLarge, overlayText, GetClientArea(),
                    style.ForegroundDisabled, TextAlignment.Center, TextAlignment.Center, textWrap);
            }

            base.Draw();
        }

        public override bool OnMouseDown(Float2 location, MouseButton button)
        {
            if (base.OnMouseDown(location, button))
            {
                return true;
            }
            if (button == MouseButton.Right)
            {
                m_IsMouseDown = true;
                return true;
            }
            return false;
        }

        public override bool OnMouseUp(Float2 location, MouseButton button)
        {
            if (base.OnMouseUp(location, button))
            {
                return true;
            }
            if (m_IsMouseDown && button == MouseButton.Right)
            {
                m_IsMouseDown = false;
                ShowContextMenu(Parent, location + m_SearchBox.BottomLeft);
                return true;
            }
            return button == MouseButton.Left;
        }

        public override void OnLostFocus()
        {
            m_IsMouseDown = false;
            base.OnLostFocus();
        }

        public override void OnDestroy()
        {
            m_SearchBox.TextChanged -= OnSearchBoxTextChanged;
            m_Tree.SelectedChanged -= TreeOnSelectedChanged;
            m_Tree.RightClick -= OnTreeRightClick;
            base.OnDestroy();
        }

        private void ScrollToSelectedNode()
        {
            List<TreeNode> selection = m_Tree.Selection;
            if (selection.Count != 0)
            {
                m_SceneTreePanel.ScrollViewTo(selection[selection.Count - 1]);
            }
        }

        private void OnSearchBoxTextChanged()
        {
            if (IsLayoutLocked)
            {
                return;
            }
            m_Tree.LockChildrenRecursive();
            m_Tree.UnlockChildrenRecursive();
            PerformLayout();
        }

        private void Rename()
        {
            List<ScenesGraphNode> selection = Editor.Scene.Selection;
            if (selection.Count != 0 && selection[0] is ActorGraphNode actor)
            {
                Editor.Scene.Select(actor);
                actor.TreeNode.StartRenaming(this, m_SceneTreePanel);
            }
        }

        private void Spawn(Type type)
        {
            SceneModule sceneModule = Editor.Scene;
            if (Activator.CreateInstance(type) is not SE.Actor actor)
            {
                return;
            }

            SE.Actor parentActor = null;
            if (sceneModule.HasSthSelected && sceneModule.Selection[0] is ActorGraphNode actorNode)
            {
                parentActor = actorNode.Actor;
                actorNode.TreeNode.Expand();
            }
            if (parentActor == null && SE.Level.ScenesCount > 0)
            {
                parentActor = SE.Level.GetScene(SE.Level.ScenesCount - 1);
            }
            if (parentActor != null)
            {
                actor.Transform = parentActor.Transform;
                actor.Name = type.FullName ?? type.Name;
            }

            sceneModule.Spawn(actor, parentActor);
            sceneModule.Select(actor);
            Rename();
        }

        private void TreeOnSelectedChanged(List<TreeNode> before, List<TreeNode> after)
        {
            if (m_IsUpdatingSelection)
            {
                return;
            }
            if (after.Count > 0)
            {
                List<ScenesGraphNode> actors = new(after.Count);
                foreach (TreeNode node in after)
                {
                    if (node is ActorTreeNode actorNode && actorNode.GetActor() != null)
                    {
                        actors.Add(actorNode.GetActorGraphNode());
                    }
                }
                Editor.Scene.Select(actors);
            }
            else
            {
                Editor.Scene.Deselect();
            }
        }

        private void OnTreeRightClick(TreeNode node, Float2 location)
        {
            ShowContextMenu(node, location);
        }

        private void OnSelectionChanged()
        {
            m_IsUpdatingSelection = true;
            List<ScenesGraphNode> selection = Editor.Scene.Selection;
            if (selection.Count == 0)
            {
                m_Tree.Deselect();
            }
            else
            {
                List<TreeNode> nodes = new(selection.Count);
                foreach (ScenesGraphNode selected in selection)
                {
                    if (selected is ActorGraphNode node)
                    {
                        nodes.Add(node.TreeNode);
                    }
                }
                m_Tree.Select(nodes);
                if (nodes.Count == 1)
                {
                    nodes[0].ExpandAllParents(true);
                    m_SceneTreePanel.ScrollViewTo(nodes[0]);
                }
            }
            m_IsUpdatingSelection = false;
        }

        private ContextMenu CreateContextMenu()
        {
            SceneModule sceneModule = Editor.Scene;
            bool hasSthSelected = sceneModule.HasSthSelected;
            bool isSingleActorSelected = sceneModule.SelectionCount == 1 &&
                sceneModule.Selection[0] is ActorGraphNode;
            bool canEditScene = SE.Level.IsAnySceneLoaded;

            ContextMenu contextMenu = new() { MinimumWidth = 120 };
            ContextMenuButton button = contextMenu.AddButton("Expand All", OnExpandAllClicked);
            button.Enabled = hasSthSelected;
            button = contextMenu.AddButton("Collapse All", OnCollapseAllClicked);
            button.Enabled = hasSthSelected;

            contextMenu.AddSeparator();
            button = contextMenu.AddButton("Rename", Rename);
            button.Enabled = isSingleActorSelected;
            button = contextMenu.AddButton("Duplicate", sceneModule.Duplicate);
            button.Enabled = hasSthSelected;
            button = contextMenu.AddButton("Delete", sceneModule.Delete);
            button.Enabled = hasSthSelected;

            contextMenu.AddSeparator();
            button = contextMenu.AddButton("Copy", sceneModule.Copy);
            button.Enabled = hasSthSelected;
            contextMenu.AddButton("Paste", sceneModule.Paste);
            button = contextMenu.AddButton("Cut", sceneModule.Cut);
            button.Enabled = canEditScene;

            contextMenu.AddSeparator();
            ContextMenuChildMenu createMenu = contextMenu.AddChildMenu("Create");
            foreach ((string path, Type actorType) in m_ActorCreateMenus)
            {
                string[] parts = path.Split('/');
                ContextMenu menu = null;
                for (int index = 0; index < parts.Length; index++)
                {
                    string part = parts[index].TrimEnd();
                    if (index == parts.Length - 1)
                    {
                        if (menu == null)
                        {
                            createMenu.ContextMenu.AddButton(part, () => Spawn(actorType));
                        }
                        else
                        {
                            menu.AddButton(part, () => Spawn(actorType));
                            menu.AutoSort = true;
                        }
                    }
                    else
                    {
                        ContextMenuChildMenu child = menu == null
                            ? createMenu.ContextMenu.GetOrAddChildMenu(part)
                            : menu.GetOrAddChildMenu(part);
                        menu = child.ContextMenu;
                        menu.AutoSort = true;
                    }
                }
            }
            return contextMenu;
        }

        private void OnExpandAllClicked(ContextMenuButton button)
        {
            foreach (ScenesGraphNode selected in Editor.Scene.Selection)
            {
                if (selected is ActorGraphNode node)
                {
                    node.TreeNode.ExpandAll();
                }
            }
        }

        private void OnCollapseAllClicked(ContextMenuButton button)
        {
            foreach (ScenesGraphNode selected in Editor.Scene.Selection)
            {
                if (selected is ActorGraphNode node)
                {
                    node.TreeNode.CollapseAll();
                }
            }
        }
    }
}
