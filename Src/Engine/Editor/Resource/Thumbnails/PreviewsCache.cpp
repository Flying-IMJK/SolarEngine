
#include "PreviewsCache.h"

#include "Runtime/Core/Thread/Threading.h"
#include "Runtime/Core/Thread/ThreadPool.h"
#include "Runtime/Graphics/GPUContext.h"
#include "Runtime/Graphics/GPUDevice.h"
#include "Runtime/Graphics/Base/GPUUtils.h"
#include "Runtime/Graphics/Textures/GPUTexture.h"
#include "Runtime/Render/Assets/Texture/TextureData.h"
#include "Runtime/Render/Utils/RenderUtils.h"
#include "Runtime/Resource/Importers/AssetsImportingSystem.h"
#include "Runtime/Resource/Factories/BinaryAssetFactory.h"

// Default asset preview icon size (both width and height since it's a square)
#define ASSET_ICON_SIZE 64

// Default assets previews atlas size
#define ASSETS_ICONS_ATLAS_SIZE 1024

// Default assets previews atlas margin between icons
#define ASSETS_ICONS_ATLAS_MARGIN 4

// Default format for assets previews atlas texture
#define ASSETS_ICONS_ATLAS_FORMAT PixelFormat::R8G8B8A8_UNorm

// Util macros
#define ASSETS_ICONS_PER_ROW (int)((float)ASSETS_ICONS_ATLAS_SIZE / (ASSET_ICON_SIZE + ASSETS_ICONS_ATLAS_MARGIN))
#define ASSETS_ICONS_PER_ATLAS (ASSETS_ICONS_PER_ROW*ASSETS_ICONS_PER_ROW)

namespace SE::Editor
{
    class FlushTask : public Threading::ThreadPoolTask
    {
    private:

        PreviewsCache* m_Cache;
        TextureData _data;

    public:

        FlushTask(PreviewsCache* cache)
            : m_Cache(cache)
        {
        }

    public:

        /// <summary>
        /// Gets the texture data container.
        /// </summary>
        TextureData& GetData()
        {
            return _data;
        }

    protected:

        // [ThreadPoolTask]
        bool Run() override;
        void OnEnd() override;
    };

    bool FlushTask::Run()
    {
        // Check if has valid data downloaded
        if (_data.GetMipLevels() != 1)
        {
            LOG_WARNING("Asset", "Failed to flush asset previews atlas \'{0}\'.", m_Cache->ToString());
            return false;
        }
        auto mipData = _data.GetData(0, 0);
        ENGINE_ASSERT(mipData->DepthPitch ==  GPUUtils::CalculateTextureMemoryUsage(m_Cache->GetTexture()->Format(), m_Cache->Width(), m_Cache->Height(), 1));

        Threading::ScopeLock lock(m_Cache->Locker);

        // Link chunks (don't allocate additional memory)
        auto mipChunk = m_Cache->GetOrCreateChunk(0);
        auto dataChunk = m_Cache->GetOrCreateChunk(15);
        mipChunk->Data.Link(mipData->Data);
        dataChunk->Data.Link((byte*)m_Cache->m_Assets.Get(), sizeof(UID) * m_Cache->m_Assets.Count());

        // Prepare asset data
        AssetInitData data;
        data.SerializedVersion = m_Cache->GetSerializedVersion();
        data.CustomData.Copy(m_Cache->m_Texture.GetHeader());

        // Save (use silent mode to prevent asset reloading)
        bool saveResult = m_Cache->SaveAsset(data, true);
        mipChunk->Data.Release();
        dataChunk->Data.Release();
        if (!saveResult)
        {
            LOG_WARNING("Asset", "Failed to save asset previews atlas \'{0}\'.", m_Cache->ToString());
            return false;
        }

        // Clear flag
        m_Cache->m_IsDirty = false;

        return true;
    }

    void FlushTask::OnEnd()
    {
        ASSERT(_cache->_flushTask == this);
        m_Cache->m_FlushTask = nullptr;

        // Base
        ThreadPoolTask::OnEnd();
    }

    BINARY_ASSET_FACTORY(PreviewsCache, true);


    PreviewsCache::PreviewsCache(const SpawnParams& params, const AssetInfo* info): SpriteAtlas(params, info)
    {
    }

    bool PreviewsCache::IsReady() const
    {
        return IsLoaded() && GetTexture()->MipLevels() > 0;
    }

    SpriteHandle PreviewsCache::FindSlot(const UID& id)
    {
        if (!WaitForLoaded())
        {
            return SpriteHandle::Invalid;
        }
        int32 index;
        if (m_Assets.Find(id, index))
        {
            const String spriteName = StringUtils::ToString(index);
            return FindSprite(spriteName);
        }
        return SpriteHandle::Invalid;
    }

    Asset::LoadResult PreviewsCache::load()
    {
        // Load previews data
        auto previewsMetaChunk = GetChunk(15);
        if (previewsMetaChunk == nullptr || !previewsMetaChunk->IsValid())
        {
            return LoadResult::MissingDataChunk;
        }
        if (previewsMetaChunk->Size() != ASSETS_ICONS_PER_ATLAS * sizeof(UID))
        {
            return LoadResult::Failed;
        }
        m_Assets.Set(previewsMetaChunk->Get<UID>(), ASSETS_ICONS_PER_ATLAS);

        // Verify if cached assets still exist (don't store thumbnails for removed files)
        AssetInfo assetInfo;
        for (UID& id : m_Assets)
        {
            if (id.IsValid() && AssetContent::GetAsset(id) == nullptr && !AssetContent::GetAssetInfo(id, assetInfo))
            {
                // Free slot (no matter the texture contents)
                id = UID::Empty;
            }
        }

        // Setup atlas sprites array
        Sprite sprite;
        sprite.Area.Size = static_cast<float>(ASSET_ICON_SIZE) / ASSETS_ICONS_ATLAS_SIZE;
        const float positionScale = static_cast<float>(ASSET_ICON_SIZE + ASSETS_ICONS_ATLAS_MARGIN) / ASSETS_ICONS_ATLAS_SIZE;
        const float positionOffset = static_cast<float>(ASSETS_ICONS_ATLAS_MARGIN) / ASSETS_ICONS_ATLAS_SIZE;
        for (int32 i = 0; i < ASSETS_ICONS_PER_ATLAS; i++)
        {
            sprite.Area.Location = Float2(static_cast<float>(i % ASSETS_ICONS_PER_ROW), static_cast<float>(i / ASSETS_ICONS_PER_ROW)) * positionScale + positionOffset;
            sprite.Name = StringUtils::ToString(i);
            Sprites.Add(sprite);
        }

        m_IsDirty = false;
        return TextureBase::load();
    }

    void PreviewsCache::Unload(bool isReloading)
    {
        // Wait for flush end
        if (IsFlushing())
        {
            m_FlushTask->Cancel();
        }

        // Release data
        m_Assets.Clear();

        SpriteAtlas::Unload(isReloading);
    }

    AssetChunksFlag PreviewsCache::GetChunksToPreload() const
    {
        // Preload previews ids data chunk
        return GET_CHUNK_FLAG(15);
    }

    bool PreviewsCache::HasFreeSlot() const
    {
        // Unused slot is which ID is Empty
        // (Search from back to front since slots are allocated from front to back - it will be faster)
        return m_Assets.FindLast(UID::Empty) != INVALID_INDEX;
    }

    SpriteHandle PreviewsCache::OccupySlot(GPUTexture* source, const UID& id)
    {
        if (!WaitForLoaded())
            return SpriteHandle::Invalid;

        // Find this asset slot or use the first empty
        int32 index = m_Assets.Find(id);
        if (index == INVALID_INDEX)
            index = m_Assets.Find(UID::Empty);
        if (index == INVALID_INDEX)
        {
            LOG_WARNING("Asset", "Cannot find free slot in the asset previews atlas.");
            return SpriteHandle::Invalid;
        }

        ENGINE_ASSERT(IsReady());

        // Copy texture region
        uint32 x = ASSETS_ICONS_ATLAS_MARGIN + index % ASSETS_ICONS_PER_ROW * (ASSET_ICON_SIZE + ASSETS_ICONS_ATLAS_MARGIN);
        uint32 y = ASSETS_ICONS_ATLAS_MARGIN + index / ASSETS_ICONS_PER_ROW * (ASSET_ICON_SIZE + ASSETS_ICONS_ATLAS_MARGIN);
        GPUDevice::instance->GetMainContext()->CopyTexture(GetTexture(), 0, x, y, 0, source, 0);

        // Occupy slot
        m_Assets[index] = id;

        // Get sprite handle
        const String spriteName = StringUtils::ToString(index);
        const auto slot = FindSprite(spriteName);
        if (!slot.IsValid())
        {
            LOG_WARNING("Asset", "Cannot create sprite handle for asset preview.");
            return SpriteHandle::Invalid;
        }

        // Set dirty flag
        m_IsDirty = true;

        return slot;
    }

    bool PreviewsCache::ReleaseSlot(const UID& id)
    {
        bool result = false;
        Threading::ScopeLock lock(Locker);
        int32 index = m_Assets.Find(id);
        if (index != INVALID_INDEX)
        {
            m_Assets[index] = UID::Empty;
            result = true;
        }
        return result;
    }

    void PreviewsCache::Flush()
    {
        Threading::ScopeLock lock(Locker);

        // Check if is fully loaded and is dirty and is not during downloading
        if (m_IsDirty && !IsFlushing() && IsReady())
        {
            // Spawn flushing tasks sequence
            m_FlushTask = New<FlushTask>(this);
            auto downloadDataTask = GetTexture()->DownloadDataAsync(m_FlushTask->GetData());
            downloadDataTask->ContinueWith(m_FlushTask);
            downloadDataTask->Start();
        }
    }


    bool PreviewsCache::Create(const StringView& outputPath)
    {
        LOG_INFO("Asset", "Creating new atlas '{0}' for assets previews cache. Size: {1}, capacity: {2}", outputPath, ASSETS_ICONS_ATLAS_SIZE, ASSETS_ICONS_PER_ATLAS);
        return AssetsImporting::Create(&Create, outputPath);
    }

    CreateAssetResult PreviewsCache::Create(CreateAssetContext& context)
    {
        // Base
        IMPORT_SETUP(PreviewsCache, ASSET_VERSION_SPRITEATLAS);

        // Create texture header (custom data)
        TextureHeader textureHeader;
        textureHeader.Width = ASSETS_ICONS_ATLAS_SIZE;
        textureHeader.Height = ASSETS_ICONS_ATLAS_SIZE;
        textureHeader.Format = ASSETS_ICONS_ATLAS_FORMAT;
        textureHeader.MipLevels = 1;
        textureHeader.NeverStream = true;
        context.Data.CustomData.Copy(&textureHeader);

        // Create blank image (chunk 0)
        uint64 imageSize = RenderUtils::CalculateTextureMemoryUsage(ASSETS_ICONS_ATLAS_FORMAT, ASSETS_ICONS_ATLAS_SIZE, ASSETS_ICONS_ATLAS_SIZE, 1);
        ENGINE_ASSERT(imageSize <= Max_int32);
        if (!context.AllocateChunk(0))
            return CreateAssetResult::CannotAllocateChunk;
        auto mipChunk = context.Data.Header.Chunks[0];
        mipChunk->Data.Allocate((int32)imageSize);
        Platform::MemoryClear(mipChunk->Get(), mipChunk->Size());

        // Create IDs cache array (chunk 15)
        int32 idsSize = sizeof(UID) * ASSETS_ICONS_PER_ATLAS;
        if (!context.AllocateChunk(15))
            return CreateAssetResult::CannotAllocateChunk;
        auto dataChunk = context.Data.Header.Chunks[15];
        dataChunk->Data.Allocate(idsSize);
        Platform::MemoryClear(dataChunk->Get(), dataChunk->Size());

        return CreateAssetResult::Ok;
    }

} // SE