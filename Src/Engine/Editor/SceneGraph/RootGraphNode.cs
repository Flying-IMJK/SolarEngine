using System;
using System.Collections.Generic;

namespace SE.Editor.SceneGraph
{
    /// <summary>
    /// Root of the editor scene graph.
    /// </summary>
    public abstract class RootGraphNode : ActorGraphNode
    {
        protected RootGraphNode(SceneGraphFactory factory)
            : this(factory, Guid.NewGuid())
        {
        }

        protected RootGraphNode(SceneGraphFactory factory, Guid id)
            : base(factory, null, id)
        {
            TreeNode.AutoFocus = false;
        }

        public override string Name => "Root";
        public override RootGraphNode Root => this;
        public override SceneGraphNode? ParentScene => null;

        public override Transform Transform
        {
            get => SE.Transform.Identity;
            set { }
        }

        public abstract IReadOnlyList<ScenesGraphNode> Selection { get; }
        public abstract void Spawn(SE.Actor actor, SE.Actor? parent);

        public virtual void OnActorChildNodesDispose(ActorGraphNode node)
        {
        }
    }
}
