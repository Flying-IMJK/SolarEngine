#pragma once
#include "ScenesGraphNode.h"
#include "Core/Templates.h"

namespace SE::Editor
{
    class ActorGraphNode;

    /// <summary>
    /// Helper base class for actor sub nodes (eg. link points, child parts).
    /// </summary>
    class ActorChildNode : public ScenesGraphNode
    {
    public:
        /// <summary>
        /// The node index.
        /// </summary>
        int Index;

        /// <summary>
        /// Gets a value indicating whether this node can be selected directly without selecting parent actor node first.
        /// </summary>
        PRO_GET(CanBeSelectedDirectly, ActorChildNode, bool, __GetCanBeSelectedDirectly);


    protected:
        String __GetName() override;
        ::SE::Transform __GetTransform() override;
        void __SetTransform(::SE::Transform value) override;

        virtual bool __GetCanBeSelectedDirectly() { return false; }
        int __GetOrderInParent() override { return Index; }
        void __SetOrderInParent(int value) override;

    public:

        /*/// <inheritdoc />
        override SceneNode ParentScene => ParentNode.ParentScene;

        /// <inheritdoc />
        override bool CanTransform => ParentNode.CanTransform;

        /// <inheritdoc />
        override bool IsActive => ParentNode.IsActive;

        /// <inheritdoc />
        override bool IsActiveInHierarchy => ParentNode.IsActiveInHierarchy;

        /// <inheritdoc />
        override bool CanDelete => false;

        /// <inheritdoc />
        override bool CanCopyPaste => false;

        /// <inheritdoc />
        override bool CanDuplicate => false;

        /// <inheritdoc />
        override bool CanDrag => false;

        /// <inheritdoc />
        override object EditableObject => ParentNode.EditableObject;

        /// <inheritdoc />
        override object UndoRecordObject => ParentNode.UndoRecordObject;*/

        /// <inheritdoc />
        void Dispose() override;

    protected:
        /// <summary>
        /// Initializes a new instance of the <see cref="ActorChildNode"/> class.
        /// </summary>
        /// <param name="id">The child id.</param>
        /// <param name="index">The child index.</param>
        ActorChildNode(UID id, int index) : ScenesGraphNode(id)
        {
            Index = index;
        }
    };

    /// <summary>
    /// Helper base class for actor sub nodes (eg. link points, child parts).
    /// </summary>
    /// <typeparam name="T">The parent actor type.</typeparam>
    /// <seealso cref="FlaxEditor.SceneGraph.SceneGraphNode" />
    /// <seealso cref="FlaxEditor.SceneGraph.ActorNode" />
    template<typename T, typename = typename TEnableIf<TIsBaseOf<ActorGraphNode, T>::Value>::Type>
    class TActorChildNode : public ActorChildNode
    {
    protected:
        /// <summary>
        /// The actor node.
        /// </summary>
        T* _node;

        /// <summary>
        /// Initializes a new instance of the <see cref="ActorChildNode{T}"/> class.
        /// </summary>
        /// <param name="node">The parent actor node.</param>
        /// <param name="id">The child id.</param>
        /// <param name="index">The child index.</param>
        TActorChildNode(T* node, UID id, int index) : ActorChildNode(id, index)
        {
            _node = node;
        }

    public:
        /// <inheritdoc />
        void OnDispose() override
        {
            _node = nullptr;

            ActorChildNode::OnDispose();
        }
    };

} // SE