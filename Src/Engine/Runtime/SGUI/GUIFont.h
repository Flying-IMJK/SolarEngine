#pragma once
#include "Runtime/API.h"
#include "Core/Math/Color.h"

#include "Imgui/imgui.h"

//-------------------------------------------------------------------------

struct ImFont;
struct ImVec4;

//-------------------------------------------------------------------------

namespace SE::GUI
{
    enum class Font : uint8
    {
        Tiny,
        TinyBold,
        Small,
        SmallBold,
        Medium,
        MediumBold,
        Large,
        LargeBold,

        NumFonts,
        Default = Medium,
    };

    //-------------------------------------------------------------------------

    struct SE_API_RUNTIME SystemFonts
    {
        static ImFont *s_fonts[(int32)Font::NumFonts];
    };

    inline ImFont *GetFont(Font font) { return SystemFonts::s_fonts[(int32)font]; }

    //-------------------------------------------------------------------------

    class SE_API_RUNTIME [[nodiscard]] ScopedFont
    {
    public:
        ScopedFont(Font font);
        ScopedFont(Color const &color);
        ScopedFont(Font font, Color const &color);
        ~ScopedFont();

        ScopedFont &operator=(ScopedFont const &) = default;

    private:
        bool m_fontApplied = false;
        bool m_colorApplied = false;
    };

    //-------------------------------------------------------------------------

    SE_API_RUNTIME inline void PushFont(Font font)
    {
        ::ImGui::PushFont(SystemFonts::s_fonts[(int8)font]);
    }

    SE_API_RUNTIME inline void PushFontAndColor(Font font, Color const &color)
    {
        ::ImGui::PushFont(SystemFonts::s_fonts[(int8)font]);
        ::ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(color.r, color.g, color.b, color.a));
    }
}

//-------------------------------------------------------------------------

#include "Fonts/MaterialDesignIcons.h"
