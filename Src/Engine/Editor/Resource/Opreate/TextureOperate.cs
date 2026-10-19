
using SE.Editor.GUI;
using SE.GUI;

using System;

namespace SE.Editor
{
    public sealed class TextureOperate : BinaryAssetOperate
    {
        private SimpleTexturePreview m_Preview;

        public override string Name => "Texture";
        public override string FileExtension => "sge";
        public override Type AssetTypeId => typeof(Texture);

        public override Color AccentColor => Color.FromRGB(0x25B84C);

        public override bool IsProxyFor(Type typeId) => typeId != null && typeof(TextureBase).IsAssignableFrom(typeId);
        public override bool CanReimport(ContentItem item) => item is BinaryAssetItem;

        public override AssetItem ConstructItem(string path, Type typeId, Guid id)
        {
            return new TextureAssetItem(path, id, typeId);
        }

        public override void OnThumbnailDrawPrepare(ThumbnailRequest request)
        {
            if (m_Preview == null)
            {
                m_Preview = new SimpleTexturePreview
                {
                    AnchorPreset = AnchorPresets.StretchAll,
                    Offsets = Margin.Zero,
                };
            }
        }

        /// <inheritdoc />
        public override void OnThumbnailDrawBegin(ThumbnailRequest request, ContainerControl guiRoot,
            GPUContext context)
        {
            m_Preview.Asset = (Texture)request.asset;
            m_Preview.Parent = guiRoot;
        }

        /// <inheritdoc />
        public override void OnThumbnailDrawEnd(ThumbnailRequest request, ContainerControl guiRoot)
        {
            m_Preview.Asset = null;
            m_Preview.Parent = null;
        }

    }
}
