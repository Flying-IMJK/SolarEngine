using System;
using System.Collections.Generic;

namespace SE.Editor.SceneGraph
{
    /// <summary>
    /// Base node for the editor scene graph. Runtime objects stay native; this
    /// type owns only editor hierarchy state and its managed lifetime.
    /// </summary>
    public abstract class ScenesGraphNode : IDisposable
    {
        private ScenesGraphNode? m_ParentNode;
        private SE.Transform m_Transform = SE.Transform.Identity;
        private int m_OrderInParent;
        private bool m_IsDisposed;

        protected ScenesGraphNode(SceneGraphFactory factory, Guid id)
        {
            Factory = factory ?? throw new ArgumentNullException(nameof(factory));
            ID = id;
            Factory.RegisterNode(this);
        }

        internal SceneGraphFactory Factory { get; }

        /// <summary>
        /// Children in scene hierarchy order.
        /// </summary>
        public List<ScenesGraphNode> ChildNodes { get; } = new();

        public abstract string Name { get; }
        public Guid ID { get; }
        public virtual RootGraphNode? Root => ParentNode?.Root;
        public virtual Transform Transform
        {
            get => m_Transform;
            set => m_Transform = value;
        }
        public abstract SceneGraphNode? ParentScene { get; }
        public virtual int OrderInParent
        {
            get => m_OrderInParent;
            set => m_OrderInParent = value;
        }
        public bool IsDisposed => m_IsDisposed;

        public virtual ScenesGraphNode? ParentNode
        {
            get => m_ParentNode;
            set
            {
                if (ReferenceEquals(m_ParentNode, value))
                    return;
                if (ReferenceEquals(this, value))
                    throw new InvalidOperationException("A scene graph node cannot parent itself.");

                m_ParentNode?.ChildNodes.Remove(this);
                m_ParentNode = value;
                if (m_ParentNode != null && !m_ParentNode.ChildNodes.Contains(this))
                    m_ParentNode.ChildNodes.Add(this);

                OnParentChanged();
            }
        }

        public abstract SE.Object? GetEditableObject();

        /// <summary>
        /// Releases this node tree and unlinks this node from its parent.
        /// </summary>
        public virtual void Dispose()
        {
            if (m_IsDisposed)
                return;

            OnDispose();
            if (m_ParentNode != null)
            {
                m_ParentNode.ChildNodes.Remove(this);
                m_ParentNode = null;
            }
        }

        /// <summary>
        /// Releases descendants and removes this node from the graph registry.
        /// </summary>
        public virtual void OnDispose()
        {
            if (m_IsDisposed)
                return;

            m_IsDisposed = true;
            ScenesGraphNode[] children = ChildNodes.ToArray();
            foreach (ScenesGraphNode child in children)
                child.OnDispose();
            ChildNodes.Clear();

            Factory.UnregisterNode(this);
        }

        protected virtual void OnParentChanged()
        {
        }
    }
}
