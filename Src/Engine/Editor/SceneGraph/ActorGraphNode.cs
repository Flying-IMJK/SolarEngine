using System;
using System.Collections.Generic;
using SE.Editor.GUI;

namespace SE.Editor.SceneGraph
{
    /// <summary>
    /// Scene graph node backed by a native Runtime actor.
    /// </summary>
    public class ActorGraphNode : ScenesGraphNode
    {
        private SE.Actor? m_Actor;
        private readonly ActorTreeNode m_TreeNode;
        private bool m_IsTreeNodeLinked;

        public ActorGraphNode(SceneGraphFactory factory, SE.Actor actor)
            : this(factory, actor, actor?.SceneObjectId ?? throw new ArgumentNullException(nameof(actor)), new ActorTreeNode())
        {
        }

        internal ActorGraphNode(SceneGraphFactory factory, SE.Actor? actor, Guid id)
            : this(factory, actor, id, new ActorTreeNode())
        {
        }

        protected ActorGraphNode(SceneGraphFactory factory, SE.Actor? actor, Guid id, ActorTreeNode treeNode)
            : base(factory, id)
        {
            m_Actor = actor;
            m_TreeNode = treeNode ?? throw new ArgumentNullException(nameof(treeNode));
        }

        public SE.Actor? Actor => m_Actor;
        public ActorTreeNode TreeNode => m_TreeNode;
        public List<ActorChildNode> ActorChildNodes { get; } = new();

        public override string Name => m_Actor?.Name ?? string.Empty;

        public override SceneGraphNode? ParentScene
        {
            get
            {
                SE.Scene? scene = m_Actor?.Scene;
                return scene != null ? Factory.FindNode(scene.SceneObjectId) as SceneGraphNode : null;
            }
        }

        public ActorGraphNode? Find(SE.Actor actor)
        {
            ArgumentNullException.ThrowIfNull(actor);
            if (ReferenceEquals(m_Actor, actor) || m_Actor?.SceneObjectId == actor.SceneObjectId)
                return this;

            foreach (ScenesGraphNode child in ChildNodes)
            {
                if (child is ActorGraphNode actorNode)
                {
                    ActorGraphNode? result = actorNode.Find(actor);
                    if (result != null)
                        return result;
                }
            }

            return null;
        }

        public ActorChildNode AddChildNode(ActorChildNode node)
        {
            ArgumentNullException.ThrowIfNull(node);
            ActorChildNodes.Add(node);
            node.ParentNode = this;
            return node;
        }

        public void DisposeChildNodes()
        {
            ActorChildNode[] childNodes = ActorChildNodes.ToArray();
            foreach (ActorChildNode child in childNodes)
                child.Dispose();
            ActorChildNodes.Clear();
        }

        public ActorGraphNode? FindChildActor(SE.Actor actor)
        {
            ArgumentNullException.ThrowIfNull(actor);
            foreach (ScenesGraphNode child in ChildNodes)
            {
                if (child is ActorGraphNode node &&
                    (ReferenceEquals(node.Actor, actor) || node.Actor?.SceneObjectId == actor.SceneObjectId))
                {
                    return node;
                }
            }

            return null;
        }

        public override SE.Object? GetEditableObject()
        {
            return m_Actor;
        }

        public void LinkTreeNode()
        {
            if (m_IsTreeNodeLinked)
                return;

            m_IsTreeNodeLinked = true;
            m_TreeNode.LinkNode(this);
        }

        internal void RefreshActor(SE.Actor actor)
        {
            m_Actor = actor ?? throw new ArgumentNullException(nameof(actor));
            m_TreeNode.UpdateText();
        }

        protected override void OnParentChanged()
        {
            m_TreeNode.OnParentChanged(ParentNode as ActorGraphNode);
        }

        public override void Dispose()
        {
            if (IsDisposed)
                return;

            m_TreeNode.Parent = null;

            m_TreeNode.Dispose();
            DisposeChildNodes();
            base.Dispose();
        }

        public override void OnDispose()
        {
            if (IsDisposed)
                return;

            ActorChildNodes.Clear();
            m_Actor = null;
            base.OnDispose();
        }
    }
}
