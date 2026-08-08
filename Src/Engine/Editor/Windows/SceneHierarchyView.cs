using System;
using System.Collections.Generic;
using SE.Editor.GUI;
using SE.GUI;

namespace SE.Editor.GUI
{
    /// <summary>
    /// Presents the managed SceneModule hierarchy using the shared editor Tree
    /// controls. It owns no Runtime objects and only mirrors SceneGraphNode data.
    /// </summary>
    public sealed class SceneHierarchyView : ContainerControl
    {
        private readonly SceneModule m_Scene;
        private readonly Tree m_Tree;
        private readonly Dictionary<Guid, ActorTreeNode> m_Nodes = new();
        private bool m_SynchronizingSelection;

        public SceneHierarchyView(SceneModule scene)
        {
            m_Scene = scene ?? throw new ArgumentNullException(nameof(scene));
            m_Tree = AddChild(new Tree(supportMultiSelect: true) { AutoSize = false });
            m_Tree.SelectedChanged += OnTreeSelectionChanged;
            m_Scene.HierarchyChanged += Refresh;
            m_Scene.SelectionChanged += SynchronizeSelection;
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
            base.OnDispose();
        }

        private void Refresh()
        {
            m_SynchronizingSelection = true;
            m_Tree.ClearNodes();
            m_Nodes.Clear();
            foreach (SceneGraphNode node in m_Scene.Root.Children)
            {
                m_Tree.AddNode(BuildNode(node));
            }
            m_SynchronizingSelection = false;
            SynchronizeSelection();
        }

        private ActorTreeNode BuildNode(SceneGraphNode model)
        {
            ActorTreeNode result = new ActorTreeNode();
            result.LinkNode(model);
            result.IsExpanded = true;
            m_Nodes.Add(model.Id, result);
            foreach (SceneGraphNode child in model.Children)
            {
                result.AddNode(BuildNode(child));
            }
            return result;
        }

        private void OnTreeSelectionChanged(IReadOnlyList<TreeNode> previous, IReadOnlyList<TreeNode> current)
        {
            if (m_SynchronizingSelection)
                return;

            List<SceneGraphNode> selected = new();
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
            foreach (SceneGraphNode model in m_Scene.SelectedNodes)
            {
                if (m_Nodes.TryGetValue(model.Id, out ActorTreeNode? node))
                    selected.Add(node);
            }
            m_Tree.Select(selected);
            m_SynchronizingSelection = false;
        }
    }
}
