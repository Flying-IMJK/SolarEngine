using System;
using System.Collections.Generic;

namespace SE.Editor.GUI
{
    /// <summary>
    /// Managed model node for an item displayed in the editor scene hierarchy.
    /// The node owns only editor presentation state; runtime objects remain owned
    /// by the generated Runtime bindings.
    /// </summary>
    public class SceneGraphNode : IDisposable
    {
        private readonly List<SceneGraphNode> m_Children = new();
        private string m_Name;
        private bool m_IsDisposed;

        public SceneGraphNode(Guid id, string name)
        {
            Id = id;
            m_Name = name ?? string.Empty;
        }

        public Guid Id { get; }
        public string Name
        {
            get => m_Name;
            internal set
            {
                string name = value ?? string.Empty;
                if (string.Equals(m_Name, name, StringComparison.Ordinal))
                    return;

                m_Name = name;
                Changed?.Invoke(this);
            }
        }

        public bool IsActive { get; internal set; } = true;
        public SceneGraphNode? Parent { get; private set; }
        public IReadOnlyList<SceneGraphNode> Children => m_Children;
        public bool IsDisposed => m_IsDisposed;

        public event Action<SceneGraphNode>? Changed;
        public event Action<SceneGraphNode, SceneGraphNode?>? ParentChanged;
        public event Action<SceneGraphNode, SceneGraphNode>? ChildAdded;
        public event Action<SceneGraphNode, SceneGraphNode>? ChildRemoved;

        internal void SetParent(SceneGraphNode? parent)
        {
            if (ReferenceEquals(Parent, parent))
                return;

            SceneGraphNode? previousParent = Parent;
            previousParent?.m_Children.Remove(this);
            if (previousParent != null)
                previousParent.ChildRemoved?.Invoke(previousParent, this);

            Parent = parent;
            if (parent != null && !parent.m_Children.Contains(this))
            {
                parent.m_Children.Add(this);
                parent.ChildAdded?.Invoke(parent, this);
            }

            ParentChanged?.Invoke(this, previousParent);
        }

        public void Dispose()
        {
            if (m_IsDisposed)
                return;

            m_IsDisposed = true;
            for (int index = m_Children.Count - 1; index >= 0; index--)
                m_Children[index].Dispose();
            m_Children.Clear();
            SetParent(null);
            Changed = null;
            ParentChanged = null;
            ChildAdded = null;
            ChildRemoved = null;
        }
    }
}
