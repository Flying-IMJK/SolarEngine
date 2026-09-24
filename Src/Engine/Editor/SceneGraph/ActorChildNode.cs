using System;

namespace SE.Editor.SceneGraph
{
    /// <summary>
    /// Base class for special actor sub-nodes such as mesh parts and link points.
    /// </summary>
    public abstract class ActorChildNode : ScenesGraphNode
    {
        protected ActorChildNode(ActorGraphNode node, Guid id, int index)
            : base(node?.Factory ?? throw new ArgumentNullException(nameof(node)), id)
        {
            Index = index;
        }

        public int Index { get; }
        public virtual bool CanBeSelectedDirectly => false;
        public override string Name => $"{ParentNode?.Name ?? string.Empty}.{Index}";

        public override Transform Transform
        {
            get => ParentNode?.Transform ?? SE.Transform.Identity;
            set
            {
                if (ParentNode != null)
                    ParentNode.Transform = value;
            }
        }

        public override int OrderInParent
        {
            get => Index;
            set { }
        }

        public override void Dispose()
        {
            if (ParentNode is ActorGraphNode parentActorNode)
                parentActorNode.ActorChildNodes.Remove(this);

            base.Dispose();
        }
    }

    /// <summary>
    /// Typed actor child node that releases its actor-node reference on disposal.
    /// </summary>
    public abstract class ActorChildNode<T> : ActorChildNode where T : ActorGraphNode
    {
        protected T? Node;

        protected ActorChildNode(T node, Guid id, int index)
            : base(node, id, index)
        {
            Node = node;
        }

        public override void OnDispose()
        {
            Node = null;
            base.OnDispose();
        }
    }
}
