#pragma once
#include "ContentOperate.h"

namespace SE
{
    class ContainerControl;
    class GPUContext;
}

namespace SE::Editor
{
    class ThumbnailRequest;
    class AssetItem;

    /// <summary>
    /// Base class for all asset proxy objects used to manage <see cref="AssetItem"/>.
    /// </summary>
    class AssetOperate : public ContentOperate
    {
        SE_DEFINE_CLASS_DEFAULT(AssetOperate, ContentOperate)
    protected:
        bool __GetIsAsset() override { return true; }

        /// <summary>
        /// Gets a value indicating whether this instance is virtual Proxy not linked to any asset.
        /// </summary>
        virtual bool GetIsVirtual();

        /// <summary>
        /// Gets the type of the asset.
        /// </summary>
        virtual TypeID GetAssetType() = 0;

        /// <summary>
        /// Initializes rendering settings for asset preview drawing for a thumbnail.
        /// </summary>
        /// <param name="preview">The asset preview.</param>
        /*void InitAssetPreview(Viewport.Previews.AssetPreview preview)
        {
            preview.RenderOnlyWithWindow = false;
            preview.UseAutomaticTaskManagement = false;
            preview.AnchorPreset = AnchorPresets.StretchAll;
            preview.Offsets = Margin.Zero;

            var task = preview.Task;
            task.Enabled = false;

            var view = task.View;
            view.IsSingleFrame = true; // Disable LOD transitions
            task.View = view;

            var eyeAdaptation = preview.PostFxVolume.EyeAdaptation;
            eyeAdaptation.Mode = EyeAdaptationMode.None;
            eyeAdaptation.OverrideFlags |= EyeAdaptationSettingsOverride.Mode;
            preview.PostFxVolume.EyeAdaptation = eyeAdaptation;
        }*/

    public:
        /// <summary>
        /// Determines whether [is virtual proxy].
        /// </summary>
        /// <returns>
        ///   <c>true</c> if [is virtual proxy]; otherwise, <c>false</c>.
        /// </returns>
        bool IsVirtualOperate();

        /// <summary>
        /// Checks if this proxy supports the given asset type id at the given path.
        /// </summary>
        /// <param name="typeID">The asset type identifier.</param>
        /// <param name="path">The asset path.</param>
        /// <returns>True if proxy supports assets of the given type id and path.</returns>
        virtual bool AcceptsAsset(TypeID typeID, StringView path);

        /// <summary>
        /// Constructs the item for the asset.
        /// </summary>
        /// <param name="path">The asset path.</param>
        /// <param name="typeID">The asset type name identifier.</param>
        /// <param name="id">The asset identifier.</param>
        /// <returns>Created item.</returns>
        virtual AssetItem* ConstructItem(StringView path, TypeID typeID, UID id) = 0;

        /// <summary>
        /// Called when thumbnail request gets prepared for drawing.
        /// </summary>
        /// <param name="request">The request.</param>
        virtual void OnThumbnailDrawPrepare(ThumbnailRequest* request)
        {
        }

        /// <summary>
        /// Determines whether thumbnail can be drawn for the specified item.
        /// </summary>
        /// <param name="request">The request.</param>
        /// <returns><c>true</c> if this thumbnail can be drawn for the specified item; otherwise, <c>false</c>.</returns>
        virtual bool CanDrawThumbnail(ThumbnailRequest* request)
        {
            return true;
        }

        /// <summary>
        /// Called when thumbnail drawing begins. Proxy should setup scene GUI for guiRoot.
        /// </summary>
        /// <param name="request">The request to render thumbnail.</param>
        /// <param name="guiRoot">The GUI root container control.</param>
        /// <param name="context">GPU context.</param>
        virtual void OnThumbnailDrawBegin(ThumbnailRequest* request, ContainerControl* guiRoot, GPUContext* context);

        /// <summary>
        /// Called when thumbnail drawing ends. Proxy should clear custom GUI from guiRoot from that should be not destroyed.
        /// </summary>
        /// <param name="request">The request to render thumbnail.</param>
        /// <param name="guiRoot">The GUI root container control.</param>
        virtual void OnThumbnailDrawEnd(ThumbnailRequest* request, ContainerControl* guiRoot)
        {
        }

        /// <summary>
        /// Called when thumbnail requests cleans data after drawing.
        /// </summary>
        /// <param name="request">The request.</param>
        virtual void OnThumbnailDrawCleanup(ThumbnailRequest* request)
        {
        }

    };

} // SE

