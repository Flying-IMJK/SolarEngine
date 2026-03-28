#pragma once

#include "Runtime/Render/2D/SpriteAtlas.h"
#include "Runtime/Resource/Importers/Types.h"

namespace SE::Editor
{
    class FlushTask;

    // <summary>
    /// Asset which contains set of asset items thumbnails (cached previews).
    /// </summary>
    class PreviewsCache final : public SpriteAtlas
    {
        SE_CLASS_DEFAULT(PreviewsCache, SpriteAtlas)
        friend class FlushTask;
    public:
        /// <summary>
        /// The default asset previews icon size (both width and height since it's a square).
        /// </summary>
        static constexpr int AssetIconSize = 64;

        /// <summary>
        /// The default assets previews atlas size
        /// </summary>
        static constexpr int AssetIconsAtlasSize = 1024;

        /// <summary>
        /// The default assets previews atlas margin (between icons)
        /// </summary>
        static constexpr int AssetIconsAtlasMargin = 4;

        /// <summary>
        /// The amount of asset icons per atlas row.
        /// </summary>
        static constexpr int AssetIconsPerRow = (int)((float)AssetIconsAtlasSize / (AssetIconSize + AssetIconsAtlasMargin));

        /// <summary>
        /// The amount of asset icons per atlas.
        /// </summary>
        static constexpr int AssetIconsPerAtlas = AssetIconsPerRow * AssetIconsPerRow;

        /// <summary>
        /// The default format of previews atlas.
        /// </summary>
        static constexpr PixelFormat AssetIconsAtlasFormat = PixelFormat::R8G8B8A8_UNorm;

    private:
        List<UID> _assets;
        bool _isDirty = false;
        FlushTask* _flushTask = nullptr;

    public:

        PreviewsCache(const AssetInfo* info);

        /// <summary>
        /// Determines whether this atlas is ready (is loaded and has texture streamed).
        /// </summary>
        bool IsReady() const;

        /// <summary>
        /// Finds the preview icon for given asset ID.
        /// </summary>
        /// <param name="id">The asset id to find preview for it.</param>
        /// <returns>The output sprite slot handle or invalid if invalid in nothing found.</returns>
        SpriteHandle FindSlot(const UID& id);

        /// <summary>
        /// Determines whether this atlas has one or more free slots for the asset preview.
        /// </summary>
        bool HasFreeSlot() const;

        /// <summary>
        /// Occupies the atlas slot.
        /// </summary>
        /// <param name="source">The source texture to insert.</param>
        /// <param name="id">The asset identifier.</param>
        /// <returns>The added sprite slot handle or invalid if invalid in failed to occupy slot.</returns>
        SpriteHandle OccupySlot(GPUTexture* source, const UID& id);

        /// <summary>
        /// Releases the used slot.
        /// </summary>
        /// <param name="id">The asset identifier.</param>
        /// <returns>True if slot has been release, otherwise it was not found.</returns>
        bool ReleaseSlot(const UID& id);

        /// <summary>
        /// Flushes atlas data from the GPU to the asset storage (saves data).
        /// </summary>
        void Flush();

        /// <summary>
        /// Determines whether this instance is flushing.
        /// </summary>
        /// <returns>True if this previews cache is flushing, otherwise false.</returns>
        FORCE_INLINE bool IsFlushing() const
        {
            return _flushTask != nullptr;
        }

        /// <summary>
        /// Creates a new atlas.
        /// </summary>
        /// <param name="outputPath">The output asset file path.</param>
        /// <returns>True if this previews cache is flushing, otherwise false.</returns>
        static bool Create(const StringView& outputPath);

    private:

        static CreateAssetResult Create(CreateAssetContext& context);

    protected:

        // [BinaryAsset]
        LoadResult load() override;
        void Unload(bool isReloading) override;
        AssetChunksFlag GetChunksToPreload() const override;
    };
} // SE

