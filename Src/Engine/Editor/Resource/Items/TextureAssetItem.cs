#nullable disable annotations

using System;
using System.Text;

namespace SE.Editor
{
    /// <summary>
    /// Implementation of <see cref="BinaryAssetItem"/> for <see cref="TextureBase"/> assets.
    /// </summary>
    /// <seealso cref="FlaxEditor.Content.BinaryAssetItem" />
    public class TextureAssetItem : BinaryAssetItem
    {
        /// <inheritdoc />
        public TextureAssetItem(string path, Guid id, Type typeID)
            : base(path, id, typeID, ContentItemSearchFilter.Texture)
        {
        }

        /// <inheritdoc />
        protected override void OnBuildTooltipText(StringBuilder sb)
        {
            base.OnBuildTooltipText(sb);

            TextureBase asset = AssetContent.Load<TextureBase>(Id, 100);
            if (asset != null)
            {
                // sb.Append("Format: ").Append(PixelFormatGetString(asset.Format())).AppendLine();
                sb.Append("Size: ").Append(asset.Width()).Append('x').Append(asset.Height());
                // if (asset.GetArraySize() != 1)
                // {
                //     sb.Append('[').Append(asset.GetArraySize()).Append(']');
                // }
                sb.AppendLine();
                // sb.Append("Mip Levels: ").Append(asset.GetMipLevels()).AppendLine();
            }
        }
    }
}
