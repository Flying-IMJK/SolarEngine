#pragma once
#include "DragHelper.hpp"
#include "DragItems.hpp"
#include "Editor/EditorApp.h"
#include "Editor/Modules/AssetDatabaseModule.h"
#include "Runtime/Resource/Asset.h"
#include "Editor/Resource/Items/AssetItem.h"

namespace SE
{
    class Asset;
}

namespace SE::Editor
{
    class DragEventArgs;
    class AssetItem;

    /// <summary>
    /// Helper class for handling <see cref="AssetItem"/> drag and drop.
    /// </summary>
    /// <seealso cref="AssetItem" />
    template<typename U>
    class DragAssetsBase : public DragHelper<AssetItem, U>
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
        DragAssetsBase(const Function<bool(AssetItem*)> &validateFunction) : DragHelper<AssetItem, U>(validateFunction)
        {
        }

        /// <inheritdoc/>
        DragData* ToDragData(AssetItem* item) override
        {
            return GetDragData(item);
        }

        /// <inheritdoc/>
        DragData* ToDragData(List<AssetItem*>& items) override
        {
            return GetDragData(items);
        }

        /// <summary>
        /// Gets the drag data.
        /// </summary>
        /// <param name="asset">The asset.</param>
        /// <returns>The data.</returns>
        static DragData* GetDragData(Asset* asset)
        {
            return DragItems<DragEventArgs>.GetDragData(EditorApp::Ins().databaseModule->Find(asset->GetID()));
        }

        /// <summary>
        /// Gets the drag data.
        /// </summary>
        /// <param name="item">The item.</param>
        /// <returns>The data.</returns>
        static DragData* GetDragData(AssetItem* item)
        {
            ENGINE_ASSERT(item != nullptr);

            return New<DragDataText>(DragPrefix + item->Path);
        }

        /// <summary>
        /// Gets the drag data.
        /// </summary>
        /// <param name="items">The items.</param>
        /// <returns>The data.</returns>
        static DragData* GetDragData(List<AssetItem*> &items)
        {
            String text = DragPrefix;
            for (int i = 0; i < items.Count(); i++)
            {
                ContentItem* item = items[i];
                text += item->Path + '\n';
            }
            return New<DragDataText>(text);
        }

        /// <summary>
        /// Tries to parse the drag data to extract <see cref="AssetItem"/> collection.
        /// </summary>
        /// <param name="data">The data.</param>
        /// <returns>Gathered objects or empty array if cannot get any valid.</returns>
        List<AssetItem*> FromDragData(DragData* data) override
        {
            DragDataText* dataText;
            if (TypeTryCast(data, dataText))
            {
                if (dataText->Text.StartsWith(DragPrefix))
                {
                    // Remove prefix and parse spitted names
                    dataText->Text.Remove(0, DragPrefix.Length());
                    List<String> paths;
                    dataText->Text.Split(SE_TEXT('\n'), paths);
                    List<AssetItem*> results = List<AssetItem*>(paths.Count());
                    for (int i = 0; i < paths.Count(); i++)
                    {
                        // Find element
                        ContentItem* contentItem = EditorApp::Ins().databaseModule->Find(paths[i]);
                        AssetItem* obj;
                        if (contentItem != nullptr && TypeTryCast(contentItem, obj))
                        {
                            results.Add(obj);
                        }
                    }

                    return results;
                }
            }

            return List<AssetItem*>();
        }
    };

    template<typename U>
    String DragAssetsBase<U>::DragPrefix = DragAssetsBase<DragEventArgs>::DragPrefix;


    /// <summary>
    /// Assets collection drag handler.
    /// </summary>
    class DragAssets : public DragAssetsBase<DragEventArgs>
    {
    public:
        /// <summary>
        /// Initializes a new instance of the <see cref="DragAssets"/> class.
        /// </summary>
        /// <param name="validateFunction">The validation function</param>
        DragAssets(const Function<bool(AssetItem*)> &validateFunction) : DragAssetsBase(validateFunction)
        {
        }
    };

} // SEH
