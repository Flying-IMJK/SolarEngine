#pragma once

#include "DragHelper.hpp"
#include "Editor/EditorApp.h"
#include "Editor/Modules/SceneModule.h"
#include "Editor/SceneGraph/ActorGraphNode.h"
#include "Runtime/Level/Actor.h"
#include "Runtime/UI/GUI/DragData.h"

namespace SE
{
    class Actor;
}

namespace SE::Editor
{
    class ActorGraphNode;
    
    /// <summary>
    /// Helper class for handling <see cref="ActorGraphNode"/> drag and drop.
    /// </summary>
    /// <seealso cref="Actor" />
    /// <seealso cref="ActorGraphNode" />
    template<typename U>
    class DragActorsBase : public DragHelper<ActorGraphNode, U>
    {
    public:
        /// <summary>
        /// The default prefix for drag data used for <see cref="ActorGraphNode"/>.
        /// </summary>
        static String DragPrefix;

        /// <summary>
        /// Creates a new DragHelper
        /// </summary>
        /// <param name="validateFunction">The validation function</param>
        DragActorsBase(const Function<bool(ActorGraphNode*)> &validateFunction) : DragHelper<ActorGraphNode, U>(validateFunction)
        {
        }

        /// <inheritdoc/>
        DragData* ToDragData(ActorGraphNode* item) override
        {
            return GetDragData(item);
        }

        /// <inheritdoc/>
        DragData* ToDragData(List<ActorGraphNode*>& items) override
        {
            return GetDragData(items);
        }

        /// <summary>
        /// Gets the drag data.
        /// </summary>
        /// <param name="actor">The actor.</param>
        /// <returns>The data.</returns>
        static DragData* GetDragData(Actor* actor)
        {
            ENGINE_ASSERT(actor != nullptr);

            return New<DragDataText>(DragPrefix + actor->GetID().ToString(UID::FormatType::N));
        }

        /// <summary>
        /// Gets the drag data.
        /// </summary>
        /// <param name="item">The item.</param>
        /// <returns>The data.</returns>
        static DragData* GetDragData(ActorGraphNode* item)
        {
            ENGINE_ASSERT(item != nullptr);

            return New<DragDataText> (DragPrefix + item->ID.operator->().ToString(UID::FormatType::N));
        }

        /// <summary>
        /// Gets the drag data.
        /// </summary>
        /// <param name="items">The items.</param>
        /// <returns>The data.</returns>
        static DragData* GetDragData(List<ActorGraphNode*>& items)
        {
            String text = DragPrefix;
            for (int i = 0; i < items.Count(); i++)
            {
                ActorGraphNode* item = items[i];
                if (item != nullptr)
                {
                    text += item->ID.operator->().ToString(UID::FormatType::N) + '\n';
                }
            }
            return New<DragDataText>(text);
        }

        /// <inheritdoc/>
        List<ActorGraphNode*> FromDragData(DragData* data) override
        {
            DragDataText* dataText;
            if (TypeTryCast(data, dataText))
            {
                if (dataText->Text.StartsWith(DragPrefix))
                {
                    // Remove prefix and parse spitted names
                    dataText->Text.Remove(0, DragPrefix.Length());
                    List<String> ids;
                    dataText->Text.Split(SE_TEXT('\n'), ids);
                    List<ActorGraphNode*> results = List<ActorGraphNode*>(ids.Count());
                    for (int i = 0; i < ids.Count(); i++)
                    {
                        // Find element
                        UID id;
                        if (UID::Parse(ids[i], id))
                        {
                            ActorGraphNode* obj = EditorApp::Ins().sceneModule->GetActorNode(id);

                            // Check it
                            if (obj != nullptr)
                            {
                                results.Add(obj);
                            }
                        }
                    }

                    return results;
                }
            }

            return List<ActorGraphNode*>();
        }
    };

    template<typename U>
    String DragActorsBase<U>::DragPrefix = SE_TEXT("ACTOR!?");

    /// <summary>
    /// Actors references collection drag handler.
    /// </summary>
    class DragActors final : public DragActorsBase<DragEventArgs>
    {
    public:
        /// <summary>
        /// Initializes a new instance of the <see cref="DragActors"/> class.
        /// </summary>
        /// <param name="validateFunction">The validation function</param>
        DragActors(const Function<bool(ActorGraphNode*)>& validateFunction) : DragActorsBase(validateFunction)
        {
        }
    };

} // SE

