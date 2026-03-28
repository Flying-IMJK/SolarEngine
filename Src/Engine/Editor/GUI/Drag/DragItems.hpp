#pragma once

#include "DragHelper.hpp"
#include "Editor/EditorApp.h"
#include "Editor/Modules/AssetDatabaseModule.h"
#include "Editor/Resource/Items/ContentItem.h"
#include "Runtime/UI/GUI/DragData.h"

namespace SE::Editor
{
    /// <summary>
    /// Helper class for handling <see cref="ContentItem"/> drag and drop.
    /// </summary>
    /// <seealso cref="ContentItem" />
    template<typename U>
    class DragItemsBase : public DragHelper<ContentItem, U>
    {
    public:
        /// <summary>
        /// The default prefix for drag data used for <see cref="ContentItem"/>.
        /// </summary>
        static String DragPrefix;

        /// <summary>
        /// Creates a new DragHelper
        /// </summary>
        /// <param name="validateFunction">The validation function</param>
        DragItemsBase(const Function<bool(ContentItem*)> &validateFunction) : DragHelper<ContentItem, U>(validateFunction)
        {
        }

        /// <summary>
        /// Gets the drag data for the given file.
        /// </summary>
        /// <param name="path">The path.</param>
        /// <returns>The data.</returns>
        DragData* ToDragData(StringView path) { return GetDragData(path); }

        /// <inheritdoc/>
        DragData* ToDragData(ContentItem* item) override { return GetDragData(item); }

        /// <inheritdoc/>
        DragData* ToDragData(List<ContentItem*> &items) override { return GetDragData(items); }

        /// <summary>
        /// Gets the drag data.
        /// </summary>
        /// <param name="path">The path.</param>
        /// <returns>The data.</returns>
        static DragData* GetDragData(StringView path)
        {
            ENGINE_ASSERT(path.Length() > 0);

            return New<DragDataText>(DragPrefix + path);
        }

        /// <summary>
        /// Gets the drag data.
        /// </summary>
        /// <param name="item">The item.</param>
        /// <returns>The data.</returns>
        static DragData* GetDragData(ContentItem* item)
        {
            ENGINE_ASSERT(item != nullptr);

            return New<DragDataText>(DragPrefix + item->Path);
        }

        /// <summary>
        /// Gets the drag data.
        /// </summary>
        /// <param name="items">The items.</param>
        /// <returns>The data.</returns>
        static DragData* GetDragData(List<ContentItem*> &items)
        {
            String text = DragPrefix;
            for (int i = 0; i < items.Count(); i++)
            {
                ContentItem* item = items[i];
                text += item->Path + '\n';
            }
            return New<DragDataText>(text);
        }

        /// <inheritdoc/>
        List<ContentItem*> FromDragData(DragData* data) override
        {
            DragDataText* dataText = const_cast<DragDataText*>(TypeTryCast<DragDataText>(data));
            if (dataText != nullptr)
            {
                if (dataText->Text.StartsWith(DragPrefix))
                {
                    // Remove prefix and parse spitted names
                    List<String> paths;
                    dataText->Text.Remove(0, DragPrefix.Length());
                    dataText->Text.Split('\n', paths);
                    List<ContentItem*> results = List<ContentItem*>(paths.Count());
                    for (int i = 0; i < paths.Count(); i++)
                    {
                        // Find element
                        ContentItem* obj = EditorApp::Ins().databaseModule->Find(paths[i]);

                        // Check it
                        if (obj != nullptr)
                            results.Add(obj);
                    }

                    return results;
                }
            }
            return List<ContentItem*>();
        }
    };

    template<typename U>
    String DragItemsBase<U>::DragPrefix = SE_TEXT("ASSET!?");

    /// <summary>
    /// Drag content items handler.
    /// </summary>
    class DragItems : public DragItemsBase<DragEventArgs>
    {
    public:
        /// <summary>
        /// Initializes a new instance of the <see cref="DragItems"/> class.
        /// </summary>
        /// <param name="validateFunction">The validation function</param>
        DragItems(Function<bool(ContentItem*)> &validateFunction) : DragItemsBase(validateFunction)
        {
        }
    };
}
