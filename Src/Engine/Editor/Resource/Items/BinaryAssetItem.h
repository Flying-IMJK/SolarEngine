#pragma once
#include "AssetItem.h"

namespace SE
{
    class Actor;
}

namespace SE::Editor
{

    // <summary>
    /// Represents binary asset item.
    /// </summary>
    /// <seealso cref="FlaxEditor.Content.AssetItem" />
    class BinaryAssetItem : public AssetItem
    {
        SE_DEFINE_CLASS_DEFAULT(BinaryAssetItem, AssetItem)
    public:
        /// <summary>
        /// Initializes a new instance of the <see cref="BinaryAssetItem"/> class.
        /// </summary>
        /// <param name="path">The asset path.</param>
        /// <param name="id">The asset identifier.</param>
        /// <param name="typeID">The asset type name identifier.</param>
        /// <param name="searchFilter">The asset type search filter type.</param>
        BinaryAssetItem(StringView path, UID id, TypeID typeID, ContentItemSearchFilter searchFilter);

        /// <summary>
        /// Gets the asset import path.
        /// </summary>
        /// <param name="importPath">The import path.</param>
        /// <returns>True if fails, otherwise false.</returns>
        bool GetImportPath(String& importPath);

        void OnReimport(UID id);

        /// <inheritdoc />
        bool IsOfType(TypeID type) override;

    private:
        ContentItemSearchFilter __GetSearchFilter() override;

        ContentItemSearchFilter m_SearchFilter;
    };

    /// <summary>
    /// Implementation of <see cref="BinaryAssetItem"/> for <see cref="TextureBase"/> assets.
    /// </summary>
    /// <seealso cref="FlaxEditor.Content.BinaryAssetItem" />
    class TextureAssetItem : public BinaryAssetItem
    {
        SE_DEFINE_CLASS_DEFAULT(TextureAssetItem, BinaryAssetItem)
    public:
        /// <inheritdoc />
        TextureAssetItem(StringView path, UID id, TypeID typeID);

    protected:
        /// <inheritdoc />
        void OnBuildTooltipText(StringBuilder sb) override;
    };

    /// <summary>
    /// Implementation of <see cref="BinaryAssetItem"/> for <see cref="Model"/> assets.
    /// </summary>
    /// <seealso cref="FlaxEditor.Content.BinaryAssetItem" />
    class ModelItem : public BinaryAssetItem
    {
        SE_DEFINE_CLASS_DEFAULT(ModelItem, BinaryAssetItem)
    public:
        /// <inheritdoc />
        ModelItem(StringView path, UID id, TypeID typeID);

        /// <inheritdoc />
        bool OnEditorDrag(void* context) override
        {
            return true;
        }

        /// <inheritdoc />
        Actor* OnEditorDrop(void* context) override;

    protected :
        /// <inheritdoc />
        void OnBuildTooltipText(StringBuilder sb) override;
    };

    /// <summary>
    /// Implementation of <see cref="BinaryAssetItem"/> for <see cref="SkinnedModel"/> assets.
    /// </summary>
    /// <seealso cref="FlaxEditor.Content.BinaryAssetItem" />
    /*class SkinnedModeItem : public BinaryAssetItem
    {
        /// <inheritdoc />
        public SkinnedModeItem(string path, ref Guid id, string typeName, Type type)
        : base(path, ref id, typeName, type, ContentItemSearchFilter.Model)
        {
        }

        /// <inheritdoc />
        public override bool OnEditorDrag(object context)
        {
            return true;
        }

        /// <inheritdoc />
        public override Actor OnEditorDrop(object context)
        {
            return new AnimatedModel { SkinnedModel = FlaxEngine.Content.LoadAsync<SkinnedModel>(ID) };
        }

        /// <inheritdoc />
        protected override void OnBuildTooltipText(StringBuilder sb)
        {
            base.OnBuildTooltipText(sb);

            var asset = FlaxEngine.Content.Load<SkinnedModel>(ID, 100);
            if (asset)
            {
                var lods = asset.LODs;
                int triangleCount = 0, vertexCount = 0;
                for (int lodIndex = 0; lodIndex < lods.Length; lodIndex++)
                {
                    var lod = lods[lodIndex];
                    for (int meshIndex = 0; meshIndex < lod.Meshes.Length; meshIndex++)
                    {
                        var mesh = lod.Meshes[meshIndex];
                        triangleCount += mesh.TriangleCount;
                        vertexCount += mesh.VertexCount;
                    }
                }
                sb.Append("LODs: ").Append(lods.Length).AppendLine();
                sb.Append("Triangles: ").Append(triangleCount.ToString("N0")).AppendLine();
                sb.Append("Vertices: ").Append(vertexCount.ToString("N0")).AppendLine();
                sb.Append("Skeleton Nodes: ").Append(asset.Nodes.Length).AppendLine();
                sb.Append("Blend Shapes: ").Append(asset.BlendShapes.Length).AppendLine();
            }
        }
    };*/

} // SE
