#include "GUIFont.h"
#include "Core/ThirdParty/Imgui/imgui.h"

//-------------------------------------------------------------------------

#ifdef SE_DEVELOPMENT
namespace SE::GUI
{
    ImFont* SystemFonts::s_fonts[(int32_t) Font::NumFonts] = { nullptr, nullptr, nullptr, nullptr };

    //-------------------------------------------------------------------------

    ScopedFont::ScopedFont( Font font )
    {
        ENGINE_ASSERT( font != Font::NumFonts && SystemFonts::s_fonts[(uint8) font] != nullptr );
        ::ImGui::PushFont( SystemFonts::s_fonts[(uint8) font] );
        m_fontApplied = true;
    }

    ScopedFont::ScopedFont( Font font, Color const& color )
    {
        ENGINE_ASSERT( font != Font::NumFonts && SystemFonts::s_fonts[(uint8) font] != nullptr );
        ::ImGui::PushFont( SystemFonts::s_fonts[(uint8) font] );
        m_fontApplied = true;
        ::ImGui::PushStyleColor( ImGuiCol_Text, color.ToFloat4() );
        m_colorApplied = true;
    }

    ScopedFont::ScopedFont( Color const& color )
    {
        ::ImGui::PushStyleColor( ImGuiCol_Text, color.ToFloat4() );
        m_colorApplied = true;
    }

    ScopedFont::~ScopedFont()
    {
        if ( m_colorApplied )
        {
            ::ImGui::PopStyleColor();
        }

        if ( m_fontApplied )
        {
            ::ImGui::PopFont();
        }
    }
}
#endif