#pragma once

#include "Core/Logging/Exceptions/InvalidOperationException.h"
#include "Core/Math/Rectangle.h"
#include "Core/Types/Property.h"
#include "Core/Types/Strings/StringView.h"
#include "Runtime/Render/Assets/Texture/TextureBase.h"
#include "Runtime/Resource/AssetRef.h"

namespace SE
{
    class SpriteAtlas;
    class GPUTexture;

    /// <summary>
    /// Contains information about single atlas slot with sprite texture.
    /// </summary>
    struct SE_API_RUNTIME Sprite
    {
        /// <summary>
        /// The normalized area of the sprite in the atlas (in range [0;1]).
        /// </summary>
        Rectangle Area;

        /// <summary>
        /// The sprite name.
        /// </summary>
        String Name;

        Sprite() : Area() {}
        Sprite(Rectangle area, StringView name) : Area(area), Name(name) {}
        Sprite(Rectangle area, String& name) : Area(area), Name(name) {}
    };

    /// <summary>
    /// Handle to sprite atlas slot with a single sprite texture.
    /// </summary>
    struct SE_API_RUNTIME SpriteHandle : ISerializable
    {
        /// <summary>
        /// Invalid sprite handle.
        /// </summary>
        static const SpriteHandle Invalid;

        /// <summary>
        /// The parent atlas.
        /// </summary>
        AssetRef<SpriteAtlas> Atlas;

        /// <summary>
        /// The atlas sprites array index.
        /// </summary>
        int32 Index = -1;

        /// <summary>
        /// Initializes a new instance of the <see cref="SpriteHandle"/> struct.
        /// </summary>
        SpriteHandle()
        {
            Index = -1;
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="SpriteHandle"/> struct.
        /// </summary>
        /// <param name="atlas">The sprite atlas.</param>
        /// <param name="index">The sprite slot index.</param>
        SpriteHandle(SpriteAtlas* atlas, int32 index) : Atlas(atlas)
        {
            Index = index;
        }

        /// <summary>
        /// Gets or sets the sprite name.
        /// </summary>
        PRO_REF(Name, SpriteHandle, String, __GetName, __SetName);

        /// <summary>
        /// Gets or sets the sprite location (in pixels).
        /// </summary>
        PRO(Location, SpriteHandle, Float2, __GetLocation, __SetLocation);

        /// <summary>
        /// Gets or sets the sprite size (in pixels).
        /// </summary>
        PRO(Size, SpriteHandle, Float2, __GetSize, __SetSize);

        /// <summary>
        /// Gets or sets the sprite area in atlas (in normalized atlas coordinates [0;1]).
        /// </summary>
        PRO_REF(Area, SpriteHandle, Rectangle, __GetArea, __SetArea);

        /// <summary>
        /// Tries to get sprite info.
        /// </summary>
        /// <param name="result">The result.</param>
        /// <returns>True if data is valid, otherwise false.</returns>
        bool GetSprite(Sprite* result) const;

        /// <summary>
        /// Returns true if sprite is valid.
        /// </summary>
        /// <returns>True if this sprite handle is valid, otherwise false.</returns>
        bool IsValid() const;

        /// <summary>
        /// Gets the sprite atlas texture.
        /// </summary>
        /// <returns>The texture object.</returns>
        GPUTexture* GetAtlasTexture() const;

        void Serialize(SerializeContext& context) override;
        void Deserialize(DeserializeContext& context) override;

    private:
        String& __GetName();
        void __SetName(String& value);

        Float2 __GetLocation();
        void __SetLocation(Float2 value);

        Float2 __GetSize();
        void __SetSize(Float2 value);

        Rectangle& __GetArea();
        void __SetArea(Rectangle& value);
    };

    #define ASSET_VERSION_SPRITEATLAS 3

    /// <summary>
    /// Sprite atlas asset that contains collection of sprites combined into a single texture.
    /// </summary>
    /// <seealso cref="TextureBase" />
    class SE_API_RUNTIME SpriteAtlas : public TextureBase
    {
        SE_CLASS_DEFAULT(SpriteAtlas, TextureBase)

    public:
        explicit SpriteAtlas(const AssetInfo* info);

        /// <summary>
        /// List with all tiles in the sprite atlas.
        /// </summary>
        List<Sprite> Sprites;

        /// <summary>
        /// Gets the sprites count.
        /// </summary>
        int32 GetSpritesCount() const;

        /// <summary>
        /// Gets the sprite data.
        /// </summary>
        /// <param name="index">The index.</param>
        /// <returns>The sprite data.</returns>
        Sprite GetSprite(int32 index) const;

        /// <summary>
        /// Gets the sprite area.
        /// </summary>
        /// <param name="index">The index.</param>
        /// <param name="result">The output sprite area.</param>
        /// <returns>The sprite data.</returns>
        void GetSpriteArea(int32 index, Rectangle& result) const;

        /// <summary>
        /// Sets the sprite data.
        /// </summary>
        /// <param name="index">The index.</param>
        /// <param name="value">The sprite data.</param>
        void SetSprite(int32 index, const Sprite& value);

        /// <summary>
        /// Finds the sprite by the name.
        /// </summary>
        /// <param name="name">The name.</param>
        /// <returns>The sprite handle.</returns>
        SpriteHandle FindSprite(const StringView& name) const;

        /// <summary>
        /// Adds the sprite.
        /// </summary>
        /// <param name="sprite">The sprite.</param>
        /// <returns>The sprite handle.</returns>
        SpriteHandle AddSprite(const Sprite& sprite);

        /// <summary>
        /// Removes the sprite.
        /// </summary>
        /// <param name="index">The sprite index.</param>
        void RemoveSprite(int32 index);

        uint32 GetSerializedVersion() const override;

#if SE_EDITOR

        /// <summary>
        /// Save the sprites (texture content won't be modified).
        /// </summary>
        /// <returns>True if cannot save, otherwise false.</returns>
        bool SaveSprites();

#endif

    protected:

        bool LoadSprites(ReadStream& stream);

        // [BinaryAsset]
        LoadResult load() override;
        void Unload(bool isReloading) override;
        AssetChunksFlag GetChunksToPreload() const override;
    };

} // SE