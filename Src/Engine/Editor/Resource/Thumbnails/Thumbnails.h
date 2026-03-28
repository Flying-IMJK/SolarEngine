#pragma once
#include "Editor/Resource/Items/ContentItem.h"
#include "Runtime/UI/GUI/ContainerControl.h"

namespace SE
{
    class GPUContext;
    class RenderTask;
}

namespace SE::Editor
{
    class AssetOperate;
    class AssetItem;
    class EditorApp;
    class PreviewRoot;
    class ThumbnailRequest;
    class PreviewsCache;

    /// <summary>
    /// Manages asset thumbnails rendering and presentation.
    /// </summary>
    /// <seealso cref="FlaxEditor.Modules.EditorModule" />
    class Thumbnails : public IContentItemOwner
    {
    public:
        /// <summary>
        /// The minimum required quality (in range [0;1]) for content streaming resources to be loaded in order to generate thumbnail for them.
        /// </summary>
        static constexpr float MinimumRequiredResourcesQuality = 0.8f;

    private:
        EditorApp* editor;
        List<PreviewsCache*> m_Cache = List<PreviewsCache*>(4);
        String _cacheFolder;
        List<ThumbnailRequest*> _requests = List<ThumbnailRequest*>(128);
        PreviewRoot* m_GuiRoot;
        DateTime _lastFlushTime;
        RenderTask* _task;
        GPUTexture* _output;
        CriticalSection _CriticalSection;

    public:
        Thumbnails(EditorApp* editor);
        ~Thumbnails();

        /// <summary>
        /// Requests the item preview.
        /// </summary>
        /// <param name="item">The item.</param>
        /// <exception cref="System.ArgumentNullException"></exception>
        void RequestPreview(ContentItem* item);

        /// <summary>
        /// Deletes the item preview from the cache.
        /// </summary>
        /// <param name="item">The item.</param>
        /// <exception cref="System.ArgumentNullException"></exception>
        void DeletePreview(ContentItem* item);

        /*internal static bool HasMinimumQuality(TextureBase asset)
        {
            if (asset.HasStreamingError)
                return true; // Don't block thumbnails queue when texture fails to stream in (eg. unsupported format)
            var mipLevels = asset.MipLevels;
            var minMipLevels = Mathf.Min(mipLevels, 7);
            return asset.IsLoaded && asset.ResidentMipLevels >= Mathf.Max(minMipLevels, (int)(mipLevels * MinimumRequiredResourcesQuality));
        }

        internal static bool HasMinimumQuality(Model asset)
        {
            if (!asset.IsLoaded)
                return false;
            var lods = asset.LODs.Length;
            var slots = asset.MaterialSlots;
            foreach (var slot in slots)
            {
                if (slot.Material && !HasMinimumQuality(slot.Material))
                    return false;
            }
            return asset.LoadedLODs >= Mathf.Max(1, (int)(lods * MinimumRequiredResourcesQuality));
        }

        internal static bool HasMinimumQuality(SkinnedModel asset)
        {
            var lods = asset.LODs.Length;
            if (asset.IsLoaded && lods == 0)
                return true; // Skeleton-only model
            var slots = asset.MaterialSlots;
            foreach (var slot in slots)
            {
                if (slot.Material && !HasMinimumQuality(slot.Material))
                    return false;
            }
            return asset.LoadedLODs >= Mathf.Max(1, (int)(lods * MinimumRequiredResourcesQuality));
        }

        internal static bool HasMinimumQuality(MaterialBase asset)
        {
            if (asset is MaterialInstance asInstance)
                return HasMinimumQuality(asInstance);
            return HasMinimumQualityInternal(asset);
        }

        internal static bool HasMinimumQuality(Material asset)
        {
            return HasMinimumQualityInternal(asset);
        }

        internal static bool HasMinimumQuality(MaterialInstance asset)
        {
            if (!HasMinimumQualityInternal(asset))
                return false;
            var baseMaterial = asset.BaseMaterial;
            return baseMaterial == null || HasMinimumQualityInternal(baseMaterial);
        }

        private static bool HasMinimumQualityInternal(MaterialBase asset)
        {
            if (!asset.IsLoaded)
                return false;
            var parameters = asset.Parameters;
            foreach (var parameter in parameters)
            {
                if (parameter.Value is TextureBase asTexture && !HasMinimumQuality(asTexture))
                    return false;
            }
            return true;
        }*/

        // IContentItemOwner
        /// <inheritdoc />
        void OnItemDeleted(ContentItem* item) override
        {
            DeletePreview(item);
        }

        /// <inheritdoc />
        void OnItemRenamed(ContentItem* item) override
        {
        }

        /// <inheritdoc />
        void OnItemReimported(ContentItem* item) override
        {
        }

        /// <inheritdoc />
        void OnItemDispose(ContentItem* item) override;

        /// <inheritdoc />
        void OnInit();

        /// <inheritdoc />
        void OnUpdate();

        /// <inheritdoc />
        void OnExit();

    private:

        void OnRender(RenderTask* task, GPUContext* context);

        void StartPreviewsQueue();

        ThumbnailRequest* FindRequest(AssetItem* item);

        void AddRequest(AssetItem* item, AssetOperate* proxy);

        void RemoveRequest(ThumbnailRequest* request);

        void RemoveRequest(AssetItem* item);

        ThumbnailRequest* GetReadyRequest(int maxChecks);


        PreviewsCache* CreateAtlas();

        void Flush();

        bool HasAllAtlasesLoaded();

        PreviewsCache* GetValidAtlas();
    };


    /// <summary>
    /// Thumbnails GUI root control.
    /// </summary>
    /// <seealso cref="FlaxEngine.GUI.ContainerControl" />
    class PreviewRoot : public ContainerControl
    {
    public:
        /// <summary>
        /// The item accent color to draw.
        /// </summary>
        Color AccentColor;

        /// <inheritdoc />
        PreviewRoot();

        /// <inheritdoc />
        void Draw() override;
    };

} // SE

