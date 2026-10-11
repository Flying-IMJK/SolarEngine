using SE.GUI;

using System;

namespace SE.Editor
{
    public sealed class ModelOperate : BinaryAssetOperate
    {
        //private ModelPreview _preview;

        public override string Name => "Model";
        public override string FileExtension => "sge";
        public override Type AssetTypeId => typeof(Model);

        public override Color AccentColor => Color.FromRGB(0xe67e22);

        public override bool IsProxyFor(Type typeId) => typeId != null && typeof(ModelBase).IsAssignableFrom(typeId);
        public override bool CanReimport(ContentItem item) => item is BinaryAssetItem;

        public override AssetItem ConstructItem(string path, Type typeId, Guid id)
        {
            return new ModelItem(path, id, typeId);
        }

/*
        /// <inheritdoc />
        public override void OnThumbnailDrawPrepare(ThumbnailRequest request)
        {
            if (_preview == null)
            {
                _preview = new ModelPreview(false)
                {
                    ScaleToFit = false,
                };
                InitAssetPreview(_preview);
            }
        }

        /// <inheritdoc />
        public override bool CanDrawThumbnail(ThumbnailRequest request)
        {
            return _preview.HasContentLoaded && ThumbnailsModule.HasMinimumQuality((Model)request.Asset);
        }

        /// <inheritdoc />
        public override void OnThumbnailDrawBegin(ThumbnailRequest request, ContainerControl guiRoot, GPUContext context)
        {
            _preview.Model = (Model)request.Asset;
            _preview.Parent = guiRoot;
            _preview.SyncBackbufferSize();
            var bounds = _preview.Model.GetBox();
            var maxSize = Math.Max(0.001f, (float)bounds.Size.MaxValue);
            _preview.ViewportCamera.SetArcBallView(bounds);
            _preview.NearPlane = Mathf.Min(10.0f, maxSize * 0.5f);
            _preview.FarPlane = Mathf.Max(1000.0f, maxSize * 2 + 100.0f);

            _preview.Task.OnDraw();
        }

        /// <inheritdoc />
        public override void OnThumbnailDrawEnd(ThumbnailRequest request, ContainerControl guiRoot)
        {
            _preview.Model = null;
            _preview.Parent = null;
        }
*/

    }
}
