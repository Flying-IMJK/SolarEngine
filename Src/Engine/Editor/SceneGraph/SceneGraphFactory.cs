using System;
using System.Collections.Generic;

namespace SE.Editor.GUI
{
    /// <summary>
    /// Owns the managed scene-graph node registry. Runtime object lookup and
    /// event subscription are deliberately supplied by generated bindings only.
    /// </summary>
    public sealed class SceneGraphFactory : IDisposable
    {
        private readonly Dictionary<Guid, SceneGraphNode> m_Nodes = new();

        public SceneGraphFactory()
        {
            Root = new ScenesRootNode();
        }

        public ScenesRootNode Root { get; }
        public IReadOnlyDictionary<Guid, SceneGraphNode> Nodes => m_Nodes;

        public SceneGraphNode? FindNode(Guid id)
        {
            return m_Nodes.TryGetValue(id, out SceneGraphNode? node) ? node : null;
        }

        public T Register<T>(T node, SceneGraphNode? parent = null) where T : SceneGraphNode
        {
            if (node == null)
                throw new ArgumentNullException(nameof(node));
            if (node.Id == Guid.Empty)
                throw new ArgumentException("Only the root node may use an empty ID.", nameof(node));
            if (m_Nodes.ContainsKey(node.Id))
                throw new InvalidOperationException($"A scene graph node is already registered for {node.Id}.");

            m_Nodes.Add(node.Id, node);
            node.SetParent(parent ?? Root);
            return node;
        }

        public bool Remove(Guid id)
        {
            if (!m_Nodes.Remove(id, out SceneGraphNode? node))
                return false;

            RemoveDescendants(node);
            node.Dispose();
            return true;
        }

        public void Dispose()
        {
            Root.Dispose();
            m_Nodes.Clear();
        }

        private void RemoveDescendants(SceneGraphNode node)
        {
            foreach (SceneGraphNode child in node.Children)
            {
                m_Nodes.Remove(child.Id);
                RemoveDescendants(child);
            }
        }
    }
}
