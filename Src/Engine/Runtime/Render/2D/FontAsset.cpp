
#include "FontAsset.h"

#include "Font.h"
#include "FontManager.h"
#include "IncludeFreeType.h"

#include "Runtime/Core/Profiler/ProfilerCPU.h"
#include "Runtime/Core/Thread/Threading.h"

#include "Runtime/Resource/Factories/BinaryAssetFactory.h"

namespace SE
{
    // REGISTER_BINARY_ASSET_WITH_UPGRADER(FontAsset, "FlaxEngine.FontAsset", FontAssetUpgrader, true);

    BINARY_ASSET_FACTORY(FontAsset, false);

    FontAsset::FontAsset(const SpawnParams& params, const AssetInfo* info): BinaryAsset(params, info), m_Face(nullptr), m_Options()
    {
    }

    String FontAsset::GetFamilyName() const
    {
        return m_Face ? String(m_Face->family_name) : String::Empty;
    }

    String FontAsset::GetStyleName() const
    {
        return m_Face ? String(m_Face->style_name) : String::Empty;
    }

    Asset::LoadResult FontAsset::load()
    {
        // Load font data
        auto chunk0 = GetChunk(0);
        if (chunk0 == nullptr || !chunk0->IsValid())
        {
            return LoadResult::MissingDataChunk;
        }
        m_FontFile.Swap(chunk0->Data);

        // Create font face
        return Init() ? LoadResult::Failed : LoadResult::Ok;
    }

    void FontAsset::Unload(bool isReloading)
    {
        // Ensure to cleanup child font objects
        if (m_Fonts.HasItems())
        {
            LOG_WARNING("Resource", "Font asset {0} is unloading but has {1} remaining font objects created", ToString(), m_Fonts.Count());
            for (auto font : m_Fonts)
            {
                font->_asset = nullptr;
                font->DeleteObject();
            }
            m_Fonts.Clear();
        }

        // Unload face
        if (m_Face)
        {
            FT_Done_Face(m_Face);
            m_Face = nullptr;
        }

        // Cleanup data
        m_FontFile.Release();
        m_VirtualBold = nullptr;
        m_VirtualItalic = nullptr;
    }

    AssetChunksFlag FontAsset::GetChunksToPreload() const
    {
        return GET_CHUNK_FLAG(0);
    }

    bool FontAsset::Init()
    {
        const FT_Error error = FT_New_Memory_Face(FontManager::GetLibrary(), m_FontFile.Get(), static_cast<FT_Long>(m_FontFile.Length()), 0, &m_Face);
        if (error)
        {
            m_Face = nullptr;
            LOG_FT_ERROR(error);
        }
        return error;
    }

    EnumFlags<FontFlags> FontAsset::GetStyle() const
    {
        EnumFlags<FontFlags> flags = FontFlags::None;
        if ((m_Face->style_flags & FT_STYLE_FLAG_ITALIC) != 0)
            flags.SetFlag(FontFlags::Italic);
        if ((m_Face->style_flags & FT_STYLE_FLAG_BOLD) != 0)
            flags.SetFlag(FontFlags::Bold);
        return flags;
    }

    void FontAsset::SetOptions(const FontOptions& value)
    {
        m_Options = value;
    }

    Font* FontAsset::CreateFont(float size)
    {
        PROFILE_CPU();

        if (!WaitForLoaded())
            return nullptr;

        Threading::ScopeLock lock(Locker);
        if (m_Face == nullptr)
            return nullptr;

        // Check if font with that size has already been created
        for (auto font : m_Fonts)
        {
            if (font->GetAsset() == this && font->GetSize() == size)
            {
                return font;
            }
        }

        return New<Font>(this, size);
    }

    FontAsset* FontAsset::GetBold()
    {
        Threading::ScopeLock lock(Locker);
        if (m_Options.Flags.IsFlag(FontFlags::Bold))
            return this;
        if (!m_VirtualBold)
        {
            m_VirtualBold = AssetContent::CreateVirtualAsset<FontAsset>();
            m_VirtualBold->Init(m_FontFile);
            auto options = m_Options;
            options.Flags.SetFlag(FontFlags::Bold);
            m_VirtualBold->SetOptions(options);
        }
        return m_VirtualBold;
    }

    FontAsset* FontAsset::GetItalic()
    {
        Threading::ScopeLock lock(Locker);
        if (m_Options.Flags.IsFlag(FontFlags::Italic))
            return this;
        if (!m_VirtualItalic)
        {
            m_VirtualItalic = AssetContent::CreateVirtualAsset<FontAsset>();
            m_VirtualItalic->Init(m_FontFile);
            auto options = m_Options;
            options.Flags.SetFlag(FontFlags::Italic);
            m_VirtualItalic->SetOptions(options);
        }
        return m_VirtualItalic;
    }

    bool FontAsset::Init(const BytesContainer& fontFile)
    {
        Threading::ScopeLock lock(Locker);
        Unload(true);
        m_FontFile.Copy(fontFile);
        return Init();
    }

#if SE_EDITOR

    bool FontAsset::Save(const StringView& path)
    {
        // Validate state
        if (!WaitForLoaded())
        {
            LOG_ERROR("Resource", "Asset loading failed. Cannot save it.");
            return false;
        }
        if (IsVirtual() && path.IsEmpty())
        {
            LOG_ERROR("Resource", "To save virtual asset asset you need to specify the target asset path location.");
            return false;
        }

        Threading::ScopeLock lock(Locker);

        AssetInitData data;
        data.SerializedVersion = 0;
        data.CustomData.Copy(&m_Options);
        auto chunk0 = GetChunk(0);
        m_FontFile.Swap(chunk0->Data);
        const bool saveResult = path.HasChars() ? SaveAsset(path, data) : SaveAsset(data, true);
        m_FontFile.Swap(chunk0->Data);
        if (saveResult)
        {
            LOG_ERROR("Resource", "Cannot save '{0}'", ToString());
            return false;
        }

        return true;
    }

#endif

    bool FontAsset::ContainsChar(Char c) const
    {
        return FT_Get_Char_Index(m_Face, c) > 0;
    }

    void FontAsset::Invalidate()
    {
        Threading::ScopeLock lock(Locker);
        for (auto font : m_Fonts)
            font->Invalidate();
    }

    uint64 FontAsset::GetMemoryUsage() const
    {
        Locker.Lock();
        uint64 result = BinaryAsset::GetMemoryUsage();
        result += sizeof(FontAsset) - sizeof(BinaryAsset);
        result += sizeof(FT_FaceRec);
        result += m_FontFile.Length();
        for (auto font : m_Fonts)
            result += sizeof(Font);
        Locker.Unlock();
        return result;
    }

    uint32 FontAsset::GetSerializedVersion() const
    {
        return ASSET_VERSION_FONT;
    }

    bool FontAsset::OnInit(AssetInitData& initData)
    {
        if (IsVirtual())
            return false;

        // Validate
        if (initData.SerializedVersion != 0)
        {
            LOG_ERROR("Resource", "Invalid serialized font asset version.");
            return false;
        }
        if (initData.CustomData.Length() != sizeof(m_Options))
        {
            LOG_ERROR("Resource", "Missing font asset header.");
            return false;
        }

        // Copy header
        Platform::MemoryCopy(&m_Options, initData.CustomData.Get(), sizeof(m_Options));

        return true;
    }
} // SE