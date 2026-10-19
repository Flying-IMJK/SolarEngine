using SE.Editor;
using SE.GUI;

using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Reflection;
using System.Security.Cryptography;
using System.Text;
using System.Threading.Tasks;

namespace SE.Editor
{
    /// <summary>
    /// Asset item object.
    /// </summary>
    public abstract class AssetItem : ContentItem
    {
        /// <summary>
        /// Gets the asset unique identifier.
        /// </summary>
        public Guid Id => m_ID;

        /// <summary>
        /// Gets the asset type identifier.
        /// </summary>
        public Type TypeId => m_TypeID;

        /// <summary>
        /// Returns true if asset is now loaded.
        /// </summary>
        public bool GetIsLoaded()
        {
            Asset asset = AssetContent.GetAsset(Id);
            return asset != null && asset.IsLoaded();
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="AssetItem"/> class.
        /// </summary>
        /// <param name="path">The asset path.</param>
        /// <param name="typeID">The asset type.</param>
        /// <param name="id">The asset identifier.</param>
        public AssetItem(string path, Type typeID, Guid id)
            : base(path)
        {
            m_TypeID = typeID;
            m_ID = id;
        }

        /// <inheritdoc />
        /*public override void OnTooltipShown(Tooltip tooltip)
        {
            base.OnTooltipShown(tooltip);

            // Inject the hook control for the double-click event (if user double-clicks on tooltip over the asset item
            // it will open that item - helps on small screens)
            var hook = tooltip.GetChild<TooltipDoubleClickHook>();
            if (hook == null)
            {
                hook = tooltip.AddChild<TooltipDoubleClickHook>();
            }
            hook.Item = this;
        }*/

        /// <inheritdoc />
        /*public override string TypeDescription
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
        public Asset LoadAsync()
        {
            return AssetContent.LoadAsync<Asset>(Id);
        }

        /// <summary>
        /// Reloads the asset (if it's loaded).
        /// </summary>
        public void Reload()
        {
            Asset asset = AssetContent.GetAsset(Id);
            if (asset != null && asset.IsLoaded())
            {
                asset.Reload();
            }
        }

        /// <summary>
        /// Determines whether asset is of the specified type (included inheritance checks).
        /// </summary>
        /// <typeparam name="T">The type to check.</typeparam>
        /// <returns> <c> true</c> if asset is of the specified type (including inherited types); otherwise, <c>
        /// false</c>.</returns>
        public bool IsOfType<T>()
        {
            return IsOfType(typeof(T));
        }

        /// <summary>
        /// Determines whether asset is of the specified type (included inheritance checks).
        /// </summary>
        /// <param name="type">The type to check.</param>
        /// <returns> <c> true</c> if asset is of the specified type (including inherited types); otherwise, <c>
        /// false</c>.</returns>
        public virtual bool IsOfType(Type type)
        {
            return false;
        }

        /// <summary>
        /// Called when user dags this item into editor viewport or scene tree node.
        /// </summary>
        /// <param name="context">The editor context (eg. editor viewport or scene tree node).</param>
        /// <returns>True if item can be dropped in, otherwise false.</returns>
        public virtual bool OnEditorDrag(object context)
        {
            return false;
        }

        /// <summary>
        /// Called when user drops the item into editor viewport or scene tree node.
        /// </summary>
        /// <param name="context">The editor context (eg. editor viewport or scene tree node).</param>
        /// <returns>The spawned object.</returns>
        public virtual Actor OnEditorDrop(object context)
        {
            // LOG_ERROR("Asset", "Asset {} doesn't support dropping into viewport.", GetType());
            return null;
        }

        /// <inheritdoc />
        public override ContentItem Find(Guid id)
        {
            return m_ID == id ? this : null;
        }

        /// <inheritdoc />
        /*public override string ToString()
        {
            return Path + ":" + Id;
        }*/

        /// <inheritdoc />
        public override ContentItemType ItemType => ContentItemType.Asset;

        protected class TooltipDoubleClickHook : Control
        {
            public AssetItem Item;

            public TooltipDoubleClickHook()
            {
                AnchorPreset = AnchorPresets.StretchAll;
                Offsets = Margin.Zero;
            }

            public override bool OnMouseDoubleClick(Float2 location, MouseButton button)
            {
                return Item.OnMouseDoubleClick(Item.PointFromScreen(PointToScreen(location)), button);
            }
        }

        /// <inheritdoc />
        // protected override bool DrawShadow => true;

        protected Type m_TypeID;
        protected Guid m_ID;
    }
}
