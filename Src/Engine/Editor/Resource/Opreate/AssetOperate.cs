using SE.GUI;

using System;

namespace SE.Editor
{
    public abstract class AssetOperate : ContentOperate
    {
        public override bool IsAsset => true;
        public virtual bool IsVirtual => false;
        public abstract Type AssetTypeId { get; }

        public override bool IsProxyFor(ContentItem item)
        {
            return item is AssetItem asset && IsProxyFor(asset.TypeId);
        }

        public override bool IsProxyFor(Type typeId)
        {
            return typeId == AssetTypeId;
        }

        public virtual bool AcceptsAsset(Type typeId, string path)
        {
            return IsProxyFor(typeId) && path.EndsWith("." + FileExtension.TrimStart('.'), StringComparison.OrdinalIgnoreCase);
        }

        public abstract AssetItem ConstructItem(string path, Type typeId, Guid id);

        /// <summary>
        /// Called when thumbnail request gets prepared for drawing.
        /// </summary>
        /// <param name="request">The request.</param>
        public virtual void OnThumbnailDrawPrepare(ThumbnailRequest request)
        {
        }

        /// <summary>
        /// Determines whether thumbnail can be drawn for the specified item.
        /// </summary>
        /// <param name="request">The request.</param>
        /// <returns><c>true</c> if this thumbnail can be drawn for the specified item; otherwise, <c>false</c>.</returns>
        public virtual bool CanDrawThumbnail(ThumbnailRequest request)
        {
            return true;
        }

        /// <summary>
        /// Called when thumbnail drawing begins. Proxy should setup scene GUI for guiRoot.
        /// </summary>
        /// <param name="request">The request to render thumbnail.</param>
        /// <param name="guiRoot">The GUI root container control.</param>
        /// <param name="context">GPU context.</param>
        public virtual void OnThumbnailDrawBegin(ThumbnailRequest request, ContainerControl guiRoot, GPUContext context)
        {
            Label label = new Label
            {
                Text = Name,
                AnchorPreset = AnchorPresets.StretchAll,
                Offsets = Margin.Zero,
                Wrapping = TextWrapping.WrapWords
            };

            guiRoot.AddChild(label);
        }

        /// <summary>
        /// Called when thumbnail drawing ends. Proxy should clear custom GUI from guiRoot from that should be not destroyed.
        /// </summary>
        /// <param name="request">The request to render thumbnail.</param>
        /// <param name="guiRoot">The GUI root container control.</param>
        public virtual void OnThumbnailDrawEnd(ThumbnailRequest request, ContainerControl guiRoot)
        {
        }

        /// <summary>
        /// Called when thumbnail requests cleans data after drawing.
        /// </summary>
        /// <param name="request">The request.</param>
        public virtual void OnThumbnailDrawCleanup(ThumbnailRequest request)
        {
        }
    }
}
