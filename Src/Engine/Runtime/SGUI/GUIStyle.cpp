#include "GUIStyle.h"

#include "Core/ThirdParty/Imgui/imgui.h"
//-------------------------------------------------------------------------

namespace SE::GUI
{
    Color const Style::s_colorGray0( Color::FromRGBA(0xFF5B5B5B) );
    Color const Style::s_colorGray1( Color::FromRGBA(0xFF4C4C4C) );
    Color const Style::s_colorGray2( Color::FromRGBA(0xFF444444) );
    Color const Style::s_colorGray3( Color::FromRGBA(0xFF3A3A3A) );
    Color const Style::s_colorGray4( Color::FromRGBA(0xFF303030) );
    Color const Style::s_colorGray5( Color::FromRGBA(0xFF2C2C2C) );
    Color const Style::s_colorGray6( Color::FromRGBA(0xFF232323) );
    Color const Style::s_colorGray7( Color::FromRGBA(0xFF1C1C1C) );
    Color const Style::s_colorGray8( Color::FromRGBA(0xFF161616) );
    Color const Style::s_colorGray9( Color::FromRGBA(0xFF111111) );
    Color const Style::s_colorText( Color::FromRGBA(0xFFFFFFFF) );
    Color const Style::s_colorTextDisabled( Color::FromRGBA(0xFF828282) );
    Color const Style::s_colorAccent0( Color::FromRGBA(0xd5d5d5FF) );
    Color const Style::s_colorAccent1( Color::FromRGBA(0x989898FF) );
    Color const Style::s_colorAccent2( Color::FromRGBA(0x6f6f6fFF) );

    //-------------------------------------------------------------------------

    void Style::Apply()
    {
        ImGuiStyle& style = ::ImGui::GetStyle();

        //-------------------------------------------------------------------------
        // Colors
        //-------------------------------------------------------------------------

        ImVec4* colors = style.Colors;

        colors[ImGuiCol_Text] = s_colorText;
        colors[ImGuiCol_TextDisabled] = s_colorTextDisabled;
        colors[ImGuiCol_TextSelectedBg] = s_colorGray0;

        colors[ImGuiCol_TitleBg] = s_colorGray9;
        colors[ImGuiCol_TitleBgActive] = s_colorGray8;
        colors[ImGuiCol_TitleBgCollapsed] = s_colorGray8;

        colors[ImGuiCol_WindowBg] = s_colorGray6;
        colors[ImGuiCol_ChildBg] = s_colorGray6;
        colors[ImGuiCol_PopupBg] = s_colorGray6;
        colors[ImGuiCol_MenuBarBg] = s_colorGray6;

        colors[ImGuiCol_Border] = s_colorGray2;
        colors[ImGuiCol_BorderShadow] = s_colorGray6;

        colors[ImGuiCol_FrameBg] = s_colorGray8;
        colors[ImGuiCol_FrameBgHovered] = s_colorGray7;
        colors[ImGuiCol_FrameBgActive] = s_colorGray5;

        colors[ImGuiCol_Tab] = s_colorGray6;
        colors[ImGuiCol_TabActive] = s_colorGray4;
        colors[ImGuiCol_TabHovered] = s_colorGray3;
        colors[ImGuiCol_TabUnfocused] = s_colorGray6;
        colors[ImGuiCol_TabUnfocusedActive] = s_colorGray5;

        colors[ImGuiCol_Header] = s_colorGray3;
        colors[ImGuiCol_HeaderHovered] = s_colorGray2;
        colors[ImGuiCol_HeaderActive] = s_colorGray1;

        colors[ImGuiCol_Separator] = s_colorGray2;
        colors[ImGuiCol_SeparatorHovered] = s_colorGray1;
        colors[ImGuiCol_SeparatorActive] = s_colorGray0;

        colors[ImGuiCol_NavHighlight] = s_colorGray1;
        colors[ImGuiCol_DockingPreview] = s_colorGray1;

        colors[ImGuiCol_ScrollbarBg] = s_colorGray6;
        colors[ImGuiCol_ScrollbarGrab] = s_colorGray3;
        colors[ImGuiCol_ScrollbarGrabHovered] = s_colorGray2;
        colors[ImGuiCol_ScrollbarGrabActive] = s_colorGray1;

        colors[ImGuiCol_SliderGrab] = s_colorGray2;
        colors[ImGuiCol_SliderGrabActive] = s_colorGray1;

        colors[ImGuiCol_ResizeGrip] = s_colorGray3;
        colors[ImGuiCol_ResizeGripHovered] = s_colorGray2;
        colors[ImGuiCol_ResizeGripActive] = s_colorGray2;

        colors[ImGuiCol_Button] = s_colorGray3;
        colors[ImGuiCol_ButtonHovered] = s_colorGray2;
        colors[ImGuiCol_ButtonActive] = s_colorGray1;

        colors[ImGuiCol_CheckMark] = s_colorAccent1;

        colors[ImGuiCol_PlotLines] = s_colorAccent2;
        colors[ImGuiCol_PlotLinesHovered] = s_colorAccent1;
        colors[ImGuiCol_PlotHistogram] = s_colorAccent2;
        colors[ImGuiCol_PlotHistogramHovered] = s_colorAccent1;

        colors[ImGuiCol_TableRowBg] = s_colorGray6;
        colors[ImGuiCol_TableRowBgAlt] = s_colorGray5;

        colors[ImGuiCol_DragDropTarget] = s_colorGray6;

        //-------------------------------------------------------------------------
        // Style
        //-------------------------------------------------------------------------

        style.FramePadding = { 6, 6 };
        style.WindowPadding = { 8, 8 };
        style.ChildBorderSize = 0.0f;
        style.TabBorderSize = 1.0f;
        style.GrabRounding = 0.0f;
        style.GrabMinSize = 8.0f;
        style.WindowRounding = 0.0f;
        style.WindowBorderSize = 1.0f;
        style.FrameRounding = 3.0f;
        style.IndentSpacing = 8;
        style.ItemSpacing = {4, 6 };
        style.TabRounding = 6.0f;
        style.ScrollbarSize = 20.0f;
        style.ScrollbarRounding = 0.0f;
        style.CellPadding = {4, 6 };
    }


	TextColorScope::TextColorScope(const Color &color)
	{
		::ImGui::PushStyleColor(ImGuiCol_Text, color);
	}

	TextColorScope::~TextColorScope()
	{
		::ImGui::PopStyleColor();
	}
}