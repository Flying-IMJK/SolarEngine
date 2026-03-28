#pragma once
#include "ActorGraphNode.h"

namespace SE::Editor
{
    /// <summary>
    /// Represents root node of the whole scene graph.
    /// </summary>
    class RootGraphNode : public ActorGraphNode
    {
    public:
        /// <summary>
        /// Called when actor child nodes get released.
        /// </summary>
        // event Action<ActorNode> ActorChildNodesDispose;

        /// <summary>
        /// Called when actor child nodes get released.
        /// </summary>
        /// <param name="node">The node.</param>
        virtual void OnActorChildNodesDispose(ActorGraphNode* node)
        {
            // ActorChildNodesDispose?.Invoke(node);
        }

    protected:
        String __GetName() override
        {
            return SE_TEXT("Root");
        }

        RootGraphNode* __GetRoot() override
        {
            return this;
        }

        ScenesGraphNode* __GetParentScene() override
        {
            return nullptr;
        }

        ::SE::Transform __GetTransform() override;
        void __SetTransform(::SE::Transform value) override;

        virtual List<ScenesGraphNode*>& __GetSelection() = 0;

    public:
        /// <summary>
        /// Spawns the specified actor.
        /// </summary>
        /// <param name="actor">The actor.</param>
        /// <param name="parent">The parent.</param>
        virtual void Spawn(::SE::Actor* actor, ::SE::Actor* parent) = 0;

        /// <summary>
        /// Gets the list of selected scene graph nodes in the editor context.
        /// </summary>
        PRO_GET_REF(Selection, RootGraphNode, List<ScenesGraphNode*>, __GetSelection);

    protected:
        /// <summary>
        /// Initializes a new instance of the <see cref="RootNode"/> class.
        /// </summary>
        RootGraphNode();

        /// <summary>
        /// Initializes a new instance of the <see cref="RootNode"/> class.
        /// </summary>
        /// <param name="id">The node id.</param>
        RootGraphNode(UID id);
    };
} // SE

