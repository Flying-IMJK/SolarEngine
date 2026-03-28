#pragma once
#include "Runtime/Resource/AssetRef.h"
#include "Runtime/Resource/BinaryAsset.h"

typedef struct FT_FaceRec_* FT_Face;

namespace SE
{
    class Font;
    class FontManager;
    class FontAsset;

    /// <summary>
    /// The font hinting used when rendering characters.
    /// </summary>
    enum class FontHinting : byte
    {
        /// <summary>
        /// Use the default hinting specified in the font.
        /// </summary>
        Default,

        /// <summary>
        /// Force the use of an automatic hinting algorithm (over the fonts native hinter).
        /// </summary>
        Auto,

        /// <summary>
        /// Force the use of an automatic light hinting algorithm, optimized for non-monochrome displays.
        /// </summary>
        AutoLight,

        /// <summary>
        /// Force the use of an automatic hinting algorithm optimized for monochrome displays.
        /// </summary>
        Monochrome,

        /// <summary>
        /// Do not use hinting. This generally generates 'blurrier' bitmap glyphs when the glyphs are rendered in any of the anti-aliased modes.
        /// </summary>
        None,
    };

    /// <summary>
    /// The font flags used when rendering characters.
    /// </summary>
    enum class FontFlags : byte
    {
        /// <summary>
        /// No options.
        /// </summary>
        None = 0,

        /// <summary>
        /// Enables using anti-aliasing for font characters. Otherwise font will use the monochrome data.
        /// </summary>
        AntiAliasing = 1,

        /// <summary>
        /// Enables artificial embolden effect.
        /// </summary>
        Bold = 2,

        /// <summary>
        /// Enables slant effect, emulating italic style.
        /// </summary>
        Italic = 4,
    };

    /// <summary>
    /// The font asset options.
    /// </summary>
    struct FontOptions
    {
        /// <summary>
        /// The hinting.
        /// </summary>
        FontHinting Hinting;

        /// <summary>
        /// The flags.
        /// </summary>
        EnumFlags<FontFlags> Flags;
    };

    #define ASSET_VERSION_FONT 0

    /// <summary>
    /// Font asset contains glyph collection and cached data used to render text.
    /// </summary>
    class SE_API_RUNTIME FontAsset final : public BinaryAsset
    {
        SE_CLASS(FontAsset, BinaryAsset);
        friend Font;

    private:
        FT_Face m_Face;
        FontOptions m_Options;
        BytesContainer m_FontFile;
        List<Font*, InlinedAllocation<32>> m_Fonts;
        AssetRef<FontAsset> m_VirtualBold;
        AssetRef<FontAsset> m_VirtualItalic;

    public:
        FontAsset();
        explicit FontAsset(const AssetInfo* info);

        /// <summary>
        /// Gets the font family name.
        /// </summary>
        String GetFamilyName() const;

        /// <summary>
        /// Gets the font style name.
        /// </summary>
        String GetStyleName() const;

        /// <summary>
        /// Gets FreeType face handle.
        /// </summary>
        FORCE_INLINE FT_Face GetFTFace() const
        {
            return m_Face;
        }

        /// <summary>
        /// Gets the font options.
        /// </summary>
        const FontOptions& GetOptions() const
        {
            return m_Options;
        }

        /// <summary>
        /// Gets the font style flags.
        /// </summary>
        EnumFlags<FontFlags> GetStyle() const;

        /// <summary>
        /// Sets the font options.
        /// </summary>
        void SetOptions(const FontOptions& value);

    public:
        /// <summary>
        /// Creates the font object of given characters size.
        /// </summary>
        /// <param name="size">The font characters size.</param>
        /// <returns>The created font object.</returns>
        Font* CreateFont(float size);

        /// <summary>
        /// Gets the font with bold style. Returns itself or creates a new virtual font asset using this font but with bold option enabled.
        /// </summary>
        /// <returns>The virtual font or this.</returns>
        FontAsset* GetBold();

        /// <summary>
        /// Gets the font with italic style. Returns itself or creates a new virtual font asset using this font but with italic option enabled.
        /// </summary>
        /// <returns>The virtual font or this.</returns>
        FontAsset* GetItalic();

        /// <summary>
        /// Initializes the font with a custom font file data.
        /// </summary>
        /// <param name="fontFile">Raw bytes with font file data.</param>
        /// <returns>True if cannot init, otherwise false.</returns>
        bool Init(const BytesContainer& fontFile);

#if SE_EDITOR
        /// <summary>
        /// Saves this asset to the file. Supported only in Editor.
        /// </summary>
        /// <param name="path">The custom asset path to use for the saving. Use empty value to save this asset to its own storage location. Can be used to duplicate asset. Must be specified when saving virtual asset.</param>
        /// <returns>True if cannot save data, otherwise false.</returns>
        bool Save(const StringView& path = StringView::Empty);
#endif

        /// <summary>
        /// Check if the font contains the glyph of a char.
        /// </summary>
        /// <param name="c">The char to test.</param>
        /// <returns>True if the font contains the glyph of the char, otherwise false.</returns>
        bool ContainsChar(Char c) const;

        /// <summary>
        /// Invalidates all cached dynamic font atlases using this font. Can be used to reload font characters after changing font asset options.
        /// </summary>
        void Invalidate();

    public:
        // [BinaryAsset]
        uint64 GetMemoryUsage() const override;

        uint32 GetSerializedVersion() const override;


    protected:
        // [BinaryAsset]
        bool OnInit(AssetInitData& initData) override;
        LoadResult load() override;
        void Unload(bool isReloading) override;
        AssetChunksFlag GetChunksToPreload() const override;

    private:
        bool Init();
    };

} // SE

