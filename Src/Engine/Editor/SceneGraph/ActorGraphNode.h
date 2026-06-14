#pragma once
#include "ScenesGraphNode.h"

namespace SE::Editor
{
    class RootGraphNode;
}

namespace SE
{
    class Actor;
}

namespace SE::Editor
{
    class ActorChildNode;
    class ActorTreeNode;

    class ActorGraphNode : public ScenesGraphNode
    {
        SE_DEFINE_CLASS(ActorGraphNode, ScenesGraphNode)
    protected:
        /// <summary>
        /// The linked actor object.
        /// </summary>
        Actor* _actor;

        /// <summary>
        /// The tree node used to present hierarchy structure in GUI.
        /// </summary>
        ActorTreeNode* _treeNode;

        String __GetName() override;
        ScenesGraphNode* __GetParentScene() override;

    public:
        /// <summary>
        /// Gets the actor.
        /// </summary>
        PRO_GET(Actor, ActorGraphNode, Actor*, __GetActor);

        /// <summary>
        /// Gets the tree node (part of the GUI).
        /// </summary>
        PRO_GET(TreeNode, ActorGraphNode, ActorTreeNode*, __GetTreeNode);


        /// <summary>
        /// The actor child nodes used to represent special parts of the actor (meshes, links, surfaces).
        /// </summary>
        List<ActorChildNode*> ActorChildNodes;

        /*/// <summary>
        /// Gets a value indicating whether this actor affects navigation.
        /// </summary>
        public virtual bool AffectsNavigation => false;

        /// <summary>
        /// Gets a value indicating whether this actor affects navigation or any of its children (recursive).
        /// </summary>
        public bool AffectsNavigationWithChildren
        {
            get
            {
                if (_actor.HasStaticFlag(StaticFlags.Navigation) && AffectsNavigation)
                    return true;
                for (var i = 0; i < ChildNodes.Count; i++)
                {
                    if (ChildNodes[i] is ActorNode actorChild && actorChild.AffectsNavigationWithChildren)
                        return true;
                }
                return false;
            }
        }*/

        /// <summary>
        /// Tries to find the tree node for the specified actor.
        /// </summary>
        /// <param name="actor">The actor.</param>
        /// <returns>Tree node or null if cannot find it.</returns>
        ActorGraphNode* Find(::SE::Actor* actor);

        /// <summary>
        /// Adds the child node.
        /// </summary>
        /// <param name="node">The node.</param>
        /// <returns>The node</returns>
        ActorChildNode* AddChildNode(ActorChildNode* node);

        /// <summary>
        /// Disposes the child nodes.
        /// </summary>
        void DisposeChildNodes();

        /// <summary>
        /// Tries to find the tree node for the specified actor in child nodes collection.
        /// </summary>
        /// <param name="actor">The actor.</param>
        /// <returns>Tree node or null if cannot find it.</returns>
        ActorGraphNode* FindChildActor(::SE::Actor* actor);

        Object* GetEditableObject() override;

        void LinkTreeNode();

        ActorGraphNode();

        /// <summary>
        /// Initializes a new instance of the <see cref="ActorNode"/> class.
        /// </summary>
        /// <param name="actor">The actor.</param>
        ActorGraphNode(::SE::Actor* actor);

        ActorGraphNode(::SE::Actor* actor, UID id);

    protected:
        /// <summary>
        /// Initializes a new instance of the <see cref="ActorNode"/> class.
        /// </summary>
        /// <param name="actor">The actor.</param>
        /// <param name="treeNode">The custom tree node.</param>
        ActorGraphNode(::SE::Actor* actor, ActorTreeNode* treeNode);

        void OnParentChanged() override;

    private:
        ::SE::Actor* __GetActor() { return _actor; }
        ActorTreeNode* __GetTreeNode() { return _treeNode; }
    };
} // SE
