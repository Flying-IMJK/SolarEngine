#nullable disable annotations

using System;
using System.Text;

namespace SE.Editor
{
    /// <summary>
    /// Implementation of <see cref="BinaryAssetItem"/> for <see cref="Model"/> assets.
    /// </summary>
    public class ModelItem : BinaryAssetItem
    {
        /// <inheritdoc />
        public ModelItem(string path, Guid id, Type typeID)
            : base(path, id, typeID, ContentItemSearchFilter.Model)
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
            StaticModel model = new StaticModel();
            // model.Model = AssetContent.LoadAsync<Model>(Id);
            return model;
        }

        /// <inheritdoc />
        protected override void OnBuildTooltipText(StringBuilder sb)
        {
            base.OnBuildTooltipText(sb);

            Model asset = AssetContent.Load<Model>(Id, 100);
            if (asset != null)
            {
                // var lods = asset.LODs;
                // int triangleCount = 0;
                // int vertexCount = 0;
                // for (int lodIndex = 0; lodIndex < lods.Count; lodIndex++)
                // {
                //     ModelLOD lod = lods[lodIndex];
                //     for (int meshIndex = 0; meshIndex < lod.Meshes.Count; meshIndex++)
                //     {
                //         Mesh mesh = lod.Meshes[meshIndex];
                //         triangleCount += mesh.GetTriangleCount();
                //         vertexCount += mesh.GetVertexCount();
                //     }
                // }
                // sb.Append("LODs: ").Append(lods.Count).AppendLine();
                // sb.Append("Triangles: ").Append(triangleCount).AppendLine();
                // sb.Append("Vertices: ").Append(vertexCount).AppendLine();
            }
        }
    }
}
