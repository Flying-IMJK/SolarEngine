using System;

namespace SE.Editor
{
    /// <summary>
    /// Represents binary asset item.
    /// </summary>
    /// <seealso cref="AssetItem" />
    public class BinaryAssetItem : AssetItem
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="BinaryAssetItem"/> class.
        /// </summary>
        /// <param name="path">The asset path.</param>
        /// <param name="id">The asset identifier.</param>
        /// <param name="typeID">The asset type name identifier.</param>
        /// <param name="searchFilter">The asset type search filter type.</param>
        public BinaryAssetItem(string path, Guid id, Type typeID, ContentItemSearchFilter searchFilter)
            : base(path, typeID, id)
        {
            m_SearchFilter = searchFilter;
        }

        /// <summary>
        /// Gets the asset import path.
        /// </summary>
        /// <param name="importPath">The import path.</param>
        /// <returns>True if fails, otherwise false.</returns>
        public bool GetImportPath(out string importPath)
        {
            // TODO: add internal call to content backend with fast import asset metadata gather (without asset loading)
            BinaryAsset asset = AssetContent.LoadAsync<BinaryAsset>(Id);
            if (asset != null && !asset.WaitForLoaded(100))
            {
                // Get meta from loaded asset
                importPath = asset.GetPath();
                return importPath.Length > 0;
            }

            importPath = string.Empty;
            return true;
        }

        public void OnReimport(Guid id)
        {
            m_ID = id;
            base.OnReimport();
        }

        /// <inheritdoc />
        public override bool IsOfType(Type type)
        {
            return TypeId.IsAssignableFrom(type);
        }

        /// <inheritdoc />
        public override ContentItemSearchFilter SearchFilter => m_SearchFilter;

        private ContentItemSearchFilter m_SearchFilter;
    }
}
