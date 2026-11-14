using SE.Editor.GUI;
using SE.GUI;

using System;

namespace SE.Editor
{
    public sealed class CubeTextureOperate : BinaryAssetOperate
    {
        private CubeTexturePreview m_Preview;

        public override string Name => "Cube Texture";
        public override string FileExtension => "sge";
        public override Type AssetTypeId => typeof(CubeTexture);
        public override Color AccentColor => Color.FromRGB(0x25B84C);

        public override bool CanReimport(ContentItem item) => item is BinaryAssetItem;

        public override AssetItem ConstructItem(string path, Type typeId, Guid id)
        {
            return new TextureAssetItem(path, id, typeId);
        }

        public override void OnThumbnailDrawPrepare(ThumbnailRequest request)
        {
            if (m_Preview == null)
            {
                m_Preview = new CubeTexturePreview
                {
                    AnchorPreset = AnchorPresets.StretchAll,
                    Offsets = Margin.Zero,
                };
            }
        }

        public override void OnThumbnailDrawBegin(ThumbnailRequest request, ContainerControl guiRoot,
            GPUContext context)
        {
            m_Preview.Asset = (CubeTexture)request.asset;
            m_Preview.Parent = guiRoot;
        }

        public override void OnThumbnailDrawEnd(ThumbnailRequest request, ContainerControl guiRoot)
        {
            m_Preview.Asset = null;
            m_Preview.Parent = null;
        }

        public override bool CanDrawThumbnail(ThumbnailRequest request)
        {
            // Wait for GPU residency before storing the rendered thumbnail in the atlas.
            return ((CubeTexture)request.asset).GetResidentMipLevels() > 0;
        }

        public override void Dispose()
        {
            if (m_Preview != null)
            {
                m_Preview.Asset = null;
                m_Preview.Dispose();
                m_Preview = null;
            }

            base.Dispose();
        }
    }
}
