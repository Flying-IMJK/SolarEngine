
#include "SpriteAtlas.h"

#include "Runtime/Core/Serialization/MemoryReadStream.h"
#include "Runtime/Core/Serialization/MemoryWriteStream.h"
#include "Runtime/Core/Thread/Threading.h"
#include "Runtime/Graphics/GPUDevice.h"
#include "Runtime/Resource/Factories/BinaryAssetFactory.h"

namespace SE
{
    const SpriteHandle SpriteHandle::Invalid = { nullptr, INVALID_INDEX };

    BINARY_ASSET_FACTORY(SpriteAtlas, true);

    bool SpriteHandle::GetSprite(Sprite* result) const
    {
        if (IsValid())
        {
            *result = Atlas->Sprites[Index];
            return true;
        }
        return false;
    }

    bool SpriteHandle::IsValid() const
    {
        return Atlas && Index >= 0 && Atlas->Sprites.Count() > Index;
    }

    GPUTexture* SpriteHandle::GetAtlasTexture() const
    {
        ENGINE_ASSERT(Atlas);
        return Atlas->GetTexture();
    }

    void SpriteHandle::Serialize(SerializeContext& context)
    {
    }

    void SpriteHandle::Deserialize(DeserializeContext& context)
    {
    }

    String& SpriteHandle::__GetName()
    {
        if (!IsValid())
        {
            LOG_ERROR("Asset", "Cannot use invalid sprite.");
            return String::Empty;
        }

        return Atlas->Sprites[Index].Name;
    }

    void SpriteHandle::__SetName(String& value)
    {
        if (!IsValid())
        {
            LOG_ERROR("Asset", "Cannot use invalid sprite.");
            return;
        }
        Atlas->Sprites[Index].Name = value;
    }

    Float2 SpriteHandle::__GetLocation()
    {
        if (!IsValid())
        {
            LOG_ERROR("Asset", "Cannot use invalid sprite.");
            return Float2::Zero;
        }
        Sprite& sprite = Atlas->Sprites[Index];
        return sprite.Area.Location * sprite.Area.Size;
    }

    void SpriteHandle::__SetLocation(Float2 value)
    {
        if (!IsValid())
        {
            LOG_ERROR("Asset", "Cannot use invalid sprite.");
            return;
        }
        Sprite& sprite = Atlas->Sprites[Index];

        Rectangle area = sprite.Area;
        area.Location = value / Atlas->Size();
        sprite.Area = area;
    }

    Float2 SpriteHandle::__GetSize()
    {
        if (!IsValid())
        {
            LOG_ERROR("Asset", "Cannot use invalid sprite.");
            return Float2::Zero;
        }

        Sprite& sprite = Atlas->Sprites[Index];

        return sprite.Area.Size * Atlas->Size();
    }

    void SpriteHandle::__SetSize(Float2 value)
    {
        if (!IsValid())
        {
            LOG_ERROR("Asset", "Cannot use invalid sprite.");
            return;
        }

        Rectangle area = __GetArea();
        area.Size = value / Atlas->Size();
        __SetArea(area);
    }

    Rectangle& SpriteHandle::__GetArea()
    {
        if (!IsValid())
        {
            LOG_ERROR("Asset", "Cannot use invalid sprite.");
            return Rectangle::Empty;
        }

        Sprite& sprite = Atlas->Sprites[Index];
        return sprite.Area;
    }

    void SpriteHandle::__SetArea(Rectangle& value)
    {
        if (!IsValid())
        {
            LOG_ERROR("Asset", "Cannot use invalid sprite.");
            return;
        }

        Sprite& sprite = Atlas->Sprites[Index];
        sprite.Area = value;
    }

    SpriteAtlas::SpriteAtlas(const SpawnParams& params, const AssetInfo* info) : TextureBase(params, info)
    {
    }

    int32 SpriteAtlas::GetSpritesCount() const
    {
        return Sprites.Count();
    }

    Sprite SpriteAtlas::GetSprite(int32 index) const
    {
        ENGINE_ASSERT(index >= 0 && index < Sprites.Count());
        return Sprites.Get()[index];
    }

    void SpriteAtlas::GetSpriteArea(int32 index, Rectangle& result) const
    {
        ENGINE_ASSERT(index >= 0 && index < Sprites.Count());
        result = Sprites.Get()[index].Area;
    }

    void SpriteAtlas::SetSprite(int32 index, const Sprite& value)
    {
        ENGINE_ASSERT(index >= 0 && index < Sprites.Count());
        Sprites.Get()[index] = value;
    }

    SpriteHandle SpriteAtlas::FindSprite(const StringView& name) const
    {
        SpriteHandle result(const_cast<SpriteAtlas*>(this), -1);
        for (int32 i = 0; i < Sprites.Count(); i++)
        {
            if (name == Sprites[i].Name)
            {
                result.Index = i;
                break;
            }
        }
        return result;
    }

    SpriteHandle SpriteAtlas::AddSprite(const Sprite& sprite)
    {
        const int32 index = Sprites.Count();
        Sprites.Add(sprite);
        return SpriteHandle(this, index);
    }

    void SpriteAtlas::RemoveSprite(int32 index)
    {
        Sprites.RemoveAt(index);
    }

    uint32 SpriteAtlas::GetSerializedVersion() const
    {
        return ASSET_VERSION_SPRITEATLAS;
    }

#if SE_EDITOR

    bool SpriteAtlas::SaveSprites()
    {
        Threading::ScopeLock lock(Locker);

        // Load whole asset
        if (!LoadChunks(ALL_ASSET_CHUNKS))
        {
            return false;
        }

        // Prepare asset data
        AssetInitData data;
        data.SerializedVersion = 4;
        data.CustomData.Copy(_texture.GetHeader());

        // Write sprites data
        MemoryWriteStream stream(1024);
        stream.WriteInt32(1); // Version
        stream.WriteInt32(Sprites.Count()); // Sprites Count
        for (Sprite& t : Sprites)
        {
            stream.Write(t.Area);
            stream.WriteString(t.Name, 49);
        }

        // Link sprites data (unlink after safe)
        auto dataChunk = GetOrCreateChunk(15);
        dataChunk->Data.Link(stream.GetHandle(), stream.GetLength());

        // Save (use silent mode to prevent asset reloading)
        bool saveResult = SaveAsset(data, true);
        dataChunk->Data.Unlink();
        if (!saveResult)
        {
            LOG_WARNING("Render", "Failed to save sprite atlas \'{0}\'.", GetPath());
            return false;
        }

        return true;
    }

#endif

    bool SpriteAtlas::LoadSprites(ReadStream& stream)
    {
        Threading::ScopeLock lock(Locker);

        // Sprites may be used on rendering thread so lock drawing for a while
#if SE_EDITOR
        GPUDevice* gpuDevice = GPUDevice::instance;
        if (gpuDevice)
        {
            gpuDevice->locker.Lock();
        }
#endif

        Sprites.Clear();

        int32 tilesVersion, tilesCount;
        stream.ReadInt32(&tilesVersion);
        if (tilesVersion != 1)
        {
#if SE_EDITOR
            if (gpuDevice)
            {
                gpuDevice->locker.Unlock();
            }
#endif
            LOG_WARNING("Render", "Invalid tiles version.");
            return false;
        }
        stream.ReadInt32(&tilesCount);
        Sprites.Resize(tilesCount);
        for (Sprite& t : Sprites)
        {
            stream.Read(t.Area);
            stream.ReadString(&t.Name, 49);
        }

#if SE_EDITOR
        if (gpuDevice)
        {
            gpuDevice->locker.Unlock();
        }
#endif
        return true;
    }

    Asset::LoadResult SpriteAtlas::load()
    {
        auto spritesDataChunk = GetChunk(15);
        if (spritesDataChunk == nullptr || !spritesDataChunk->IsValid())
            return LoadResult::MissingDataChunk;
        MemoryReadStream spritesData(spritesDataChunk->Get(), spritesDataChunk->Size());

        if (!LoadSprites(spritesData))
        {
            LOG_WARNING("Render", "Cannot load sprites atlas data.");
            return LoadResult::Failed;
        }

        return TextureBase::load();
    }

    void SpriteAtlas::Unload(bool isReloading)
    {
        Sprites.Resize(0);

        // Base
        TextureBase::Unload(isReloading);
    }

    AssetChunksFlag SpriteAtlas::GetChunksToPreload() const
    {
        return GET_CHUNK_FLAG(15);
    }
} // SE