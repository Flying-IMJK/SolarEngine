#pragma once

#include "ContentItem.h"

namespace SE
{
    class Actor;
}

namespace SE::Editor
{
    /// <summary>
    /// Asset item object.
    /// </summary>
    class AssetItem : public ContentItem
    {
        SE_DEFINE_CLASS_DEFAULT(AssetItem, ContentItem)
    public:
        /// <summary>
        /// Gets the asset unique identifier.
        /// </summary>
        PRO_GET(id, AssetItem, UID, __GetID);

        /// <summary>
        /// Gets the asset type identifier.
        /// </summary>
        PRO_GET(typeID, AssetItem, TypeID, __GetTypeID);

        /// <summary>
        /// Returns true if asset is now loaded.
        /// </summary>
        bool GetIsLoaded();

        /// <summary>
        /// Initializes a new instance of the <see cref="AssetItem"/> class.
        /// </summary>
        /// <param name="path">The asset path.</param>
        /// <param name="typeID">The asset type.</param>
        /// <param name="id">The asset identifier.</param>
        AssetItem(StringView path, TypeID typeID, UID id);

        /// <inheritdoc />
        /*public override void OnTooltipShown(Tooltip tooltip)
        {
            base.OnTooltipShown(tooltip);

            // Inject the hook control for the double-click event (if user double-clicks on tooltip over the asset item it will open that item - helps on small screens)
            var hook = tooltip.GetChild<TooltipDoubleClickHook>();
            if (hook == null)
                hook = tooltip.AddChild<TooltipDoubleClickHook>();
            hook.Item = this;
        }*/

        /// <inheritdoc />
        /*override string TypeDescription
        {
            get
            {
                // Translate asset type name
                var typeName = TypeName;
                string[] typeNamespaces = typeName.Split('.');
                if (typeNamespaces.Length != 0 && typeNamespaces.Length != 0)
                {
                    typeName = Utilities.Utils.GetPropertyNameUI(typeNamespaces[typeNamespaces.Length - 1]);
                }
                return typeName;
            }
        }*/

        /// <summary>
        /// Loads the asset.
        /// </summary>
        /// <returns>The asset object.</returns>
        Asset* LoadAsync();

        /// <summary>
        /// Reloads the asset (if it's loaded).
        /// </summary>
        void Reload();

        /// <summary>
        /// Determines whether asset is of the specified type (included inheritance checks).
        /// </summary>
        /// <typeparam name="T">The type to check.</typeparam>
        /// <returns><c>true</c> if asset is of the specified type (including inherited types); otherwise, <c>false</c>.</returns>
        template<typename T>
        bool IsOfType()
        {
            return IsOfType(Typeof<T>());
        }

        /// <summary>
        /// Determines whether asset is of the specified type (included inheritance checks).
        /// </summary>
        /// <param name="type">The type to check.</param>
        /// <returns><c>true</c> if asset is of the specified type (including inherited types); otherwise, <c>false</c>.</returns>
        virtual bool IsOfType(TypeID type)
        {
            return false;
        }

        /// <summary>
        /// Called when user dags this item into editor viewport or scene tree node.
        /// </summary>
        /// <param name="context">The editor context (eg. editor viewport or scene tree node).</param>
        /// <returns>True if item can be dropped in, otherwise false.</returns>
        virtual bool OnEditorDrag(void* context)
        {
            return false;
        }

        /// <summary>
        /// Called when user drops the item into editor viewport or scene tree node.
        /// </summary>
        /// <param name="context">The editor context (eg. editor viewport or scene tree node).</param>
        /// <returns>The spawned object.</returns>
        virtual Actor* OnEditorDrop(void* context)
        {
            // LOG_ERROR("Asset", "Asset {} doesn't support dropping into viewport.", GetType());
            return nullptr;
        }

        /// <inheritdoc />
        ContentItem* Find(UID id) override;

        /// <inheritdoc />
        /*String ToString() override
        {
            return Path + ":" + ID;
        }*/

    protected:

        class TooltipDoubleClickHook : public Control
        {
        public:
            AssetItem* Item;

            TooltipDoubleClickHook();

            bool OnMouseDoubleClick(Float2 location, MouseButton button) override;
        };

        TypeID __GetTypeID() { return m_TypeID; }

        UID __GetID() { return m_ID; }

        ContentItemType __GetItemType() override;

        /// <inheritdoc />
        // protected override bool DrawShadow => true;

        TypeID m_TypeID;
        UID m_ID;
    };

} // SE

