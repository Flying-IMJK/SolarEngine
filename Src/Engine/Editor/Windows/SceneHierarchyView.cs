using System;
using System.Collections.Generic;
using SE.Editor.GUI;
using SE.Editor.SceneGraph;
using SE.GUI;

namespace SE.Editor.GUI
{
    /// <summary>
    /// Presents the managed SceneModule hierarchy using the shared editor Tree
    /// controls. It owns no Runtime objects and only mirrors ScenesGraphNode data.
    /// </summary>
    public sealed class SceneHierarchyView : ContainerControl
    {
        private readonly SceneModule m_Scene;
        private readonly Tree m_Tree;
        private bool m_SynchronizingSelection;

        public SceneHierarchyView(SceneModule scene)
        {
            m_Scene = scene ?? throw new ArgumentNullException(nameof(scene));
            m_Tree = AddChild(new Tree(supportMultiSelect: true) { AutoSize = false });
            m_Tree.SelectedChanged += OnTreeSelectionChanged;
            m_Scene.HierarchyChanged += Refresh;
            m_Scene.SelectionChanged += SynchronizeSelection;

            // The scene graph owns one TreeNode per actor. Mount that tree
            // directly so expansion, reparenting, and ordering stay stateful.
            ActorTreeNode rootTreeNode = m_Scene.Root.TreeNode;
            rootTreeNode.HeaderHeight = 0.0f;
            rootTreeNode.ChildrenIndent = 0.0f;
            rootTreeNode.Expand(true);
            rootTreeNode.Parent = m_Tree;
            Refresh();
        }

        protected override void OnLayoutChildren()
        {
            m_Tree.SetBounds(0.0f, 0.0f, Width, Height);
            base.OnLayoutChildren();
        }

        protected override void OnDispose()
        {
            m_Tree.SelectedChanged -= OnTreeSelectionChanged;
            m_Scene.HierarchyChanged -= Refresh;
            m_Scene.SelectionChanged -= SynchronizeSelection;
            m_Scene.Root.TreeNode.Parent = null;
            base.OnDispose();
        }

        private void Refresh()
        {
            m_Tree.PerformLayout();
            SynchronizeSelection();
        }

        private void OnTreeSelectionChanged(IReadOnlyList<TreeNode> previous, IReadOnlyList<TreeNode> current)
        {
            if (m_SynchronizingSelection)
                return;

            List<ScenesGraphNode> selected = new();
            foreach (TreeNode node in current)
            {
                if (node is ActorTreeNode actorNode && actorNode.ActorGraphNode != null)
                    selected.Add(actorNode.ActorGraphNode);
            }
            m_Scene.Select(selected);
        }

        private void SynchronizeSelection()
        {
            m_SynchronizingSelection = true;
            List<TreeNode> selected = new();
            foreach (ScenesGraphNode model in m_Scene.SelectedNodes)
            {
                if (model is ActorGraphNode actorNode && !actorNode.TreeNode.IsDisposed)
                    selected.Add(actorNode.TreeNode);
            }
            m_Tree.Select(selected);
            m_SynchronizingSelection = false;
        }
    }
}
