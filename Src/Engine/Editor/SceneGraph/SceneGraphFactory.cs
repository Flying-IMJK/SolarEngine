using System;
using System.Collections.Generic;

namespace SE.Editor.SceneGraph
{
    /// <summary>
    /// Owns node registration and all actor-to-node construction rules.
    /// </summary>
    public sealed class SceneGraphFactory : IDisposable
    {
        public sealed class NodeFactory
        {
            public NodeFactory(Type nodeType, Func<SE.Actor, ActorGraphNode> create)
            {
                NodeType = nodeType;
                Create = create;
            }

            public Type NodeType { get; }
            public Func<SE.Actor, ActorGraphNode> Create { get; }
        }

        private readonly Dictionary<Type, NodeFactory> m_CustomNodeTypes = new();
        private readonly Dictionary<Guid, ScenesGraphNode> m_Nodes = new();
        private RootGraphNode? m_Root;
        private bool m_IsDisposed;

        public SceneGraphFactory()
        {
            RegisterActorNode<SE.Scene, SceneGraphNode>(actor => new SceneGraphNode(this, (SE.Scene)actor));
            RegisterActorNode<SE.Camera, CameraGraphNode>(actor => new CameraGraphNode(this, actor));
            RegisterActorNode<SE.StaticModel, StaticModelGraphNode>(actor => new StaticModelGraphNode(this, actor));
            RegisterActorNode<SE.Sky, SkyGraphNode>(actor => new SkyGraphNode(this, actor));
            RegisterActorNode<SE.DirectionalLight, DirectionalLightGraphNode>(actor => new DirectionalLightGraphNode(this, actor));
            RegisterActorNode<SE.PointLight, PointLightGraphNode>(actor => new PointLightGraphNode(this, actor));
        }

        public IReadOnlyDictionary<Guid, ScenesGraphNode> Nodes => m_Nodes;
        public IReadOnlyDictionary<Type, NodeFactory> CustomNodeTypes => m_CustomNodeTypes;
        public RootGraphNode Root => m_Root ?? throw new InvalidOperationException("The scene graph root has not been assigned.");

        public void SetRoot(RootGraphNode root)
        {
            ArgumentNullException.ThrowIfNull(root);
            if (m_Root != null && !ReferenceEquals(m_Root, root))
                throw new InvalidOperationException("The scene graph root can only be assigned once.");
            if (!ReferenceEquals(root.Factory, this))
                throw new ArgumentException("The root belongs to another scene graph factory.", nameof(root));

            m_Root = root;
        }

        public void RegisterActorNode<TActor, TNode>(Func<SE.Actor, TNode> create)
            where TActor : SE.Actor
            where TNode : ActorGraphNode
        {
            ArgumentNullException.ThrowIfNull(create);
            m_CustomNodeTypes[typeof(TActor)] = new NodeFactory(typeof(TNode), actor => create(actor));
        }

        public ScenesGraphNode? FindNode(Guid id)
        {
            if (id == Guid.Empty)
                return null;
            return m_Nodes.TryGetValue(id, out ScenesGraphNode? node) ? node : null;
        }

        public ScenesGraphNode? GetNode(Guid id)
        {
            return FindNode(id);
        }

        public SceneGraphNode BuildSceneTree(SE.Scene scene)
        {
            ArgumentNullException.ThrowIfNull(scene);
            return (SceneGraphNode)BuildActorNode(scene);
        }

        public ActorGraphNode BuildActorNode(SE.Actor actor)
        {
            ArgumentNullException.ThrowIfNull(actor);

            ActorGraphNode result = m_CustomNodeTypes.TryGetValue(actor.GetType(), out NodeFactory? nodeFactory)
                ? nodeFactory.Create(actor)
                : new ActorGraphNode(this, actor);

            result.LinkTreeNode();
            for (int index = 0; index < actor.ChildrenCount; index++)
            {
                SE.Actor child = actor.GetChild(index);
                if (child == null)
                    continue;

                ActorGraphNode childNode = BuildActorNode(child);
                childNode.ParentNode = result;
            }

            return result;
        }

        public bool Remove(Guid id)
        {
            if (!m_Nodes.TryGetValue(id, out ScenesGraphNode? node))
                return false;

            node.Dispose();
            return true;
        }

        public void Dispose()
        {
            if (m_IsDisposed)
                return;

            m_IsDisposed = true;
            m_Root?.Dispose();
            m_Nodes.Clear();
            m_CustomNodeTypes.Clear();
            m_Root = null;
        }

        internal void RegisterNode(ScenesGraphNode node)
        {
            if (m_IsDisposed)
                throw new ObjectDisposedException(nameof(SceneGraphFactory));

            if (m_Nodes.TryGetValue(node.ID, out ScenesGraphNode? duplicate) && duplicate != null)
            {
                SE.Debug.LogWarning(
                    $"Duplicated Scene Graph node with ID {node.ID} of type '{duplicate.GetType().FullName}'.");
            }

            m_Nodes[node.ID] = node;
        }

        internal void UnregisterNode(ScenesGraphNode node)
        {
            if (m_Nodes.TryGetValue(node.ID, out ScenesGraphNode? registered) && ReferenceEquals(registered, node))
                m_Nodes.Remove(node.ID);
        }
    }
}
