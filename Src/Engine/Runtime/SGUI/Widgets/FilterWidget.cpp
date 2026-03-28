#include "FilterWidget.h"
#include "Runtime/SGUI/GUILayout.h"
#include "Runtime/SGUI/GUIStyle.h"
#include "Runtime/SGUI/Fonts/MaterialDesignIcons.h"
#include "Core/Memory/Memory.h"

#include "Imgui/imgui.h"

#ifdef SE_DEVELOPMENT

namespace SE::GUI
{
    bool FilterWidget::UpdateAndDraw(float width, EnumFlags<Flags> flags)
    {
        bool filterUpdated = false;
        ::ImGui::PushID(this);

        //-------------------------------------------------------------------------

        ::ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
        ::ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, ::ImGui::GetStyle().FrameRounding);
        ::ImGui::PushStyleColor(ImGuiCol_ChildBg, ::ImGui::GetStyle().Colors[ImGuiCol_FrameBg]);
        if (::ImGui::BeginChild("FilterLayout", ImVec2(width, ::ImGui::GetFrameHeight()), false/*, ImGuiWindowFlags_AlwaysUseWindowPadding | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NavFlattened*/))
        {
            if (flags.IsFlag(Flags::TakeInitialFocus))
            {
                if (::ImGui::IsWindowAppearing())
                {
                    ::ImGui::SetKeyboardFocusHere();
                }
            }

            //-------------------------------------------------------------------------

            ::ImGui::PushStyleColor(ImGuiCol_FrameBg, 0);

            ImVec2 const initialCursorPos = ::ImGui::GetCursorPos();

            // Draw filter input
            float const textInputWidth = ((width < 0) ? ::ImGui::GetContentRegionAvail().x : width) - (26 + ::ImGui::GetStyle().ItemSpacing.x);
            ::ImGui::SetNextItemWidth(textInputWidth);
            if (ImGui::InputText("##Filter", m_buffer))
            {
                OnBufferUpdated();
                filterUpdated = true;
            }

            // Draw clear button
            bool const isInputFocused = ::ImGui::IsItemFocused() && ::ImGui::IsItemActive();
            bool const isbufferEmpty = m_buffer.Length() == 0;
            if (!isbufferEmpty)
            {
                ::ImGui::SameLine();
                if (ImGui::ColoredButton(Colors::Transparent, Style::s_colorText, SE_TEXT(ICON_CLOSE"##Clear"), Float2(26, 24)))
                {
                    Clear();
                    filterUpdated = true;
                }
            }

            // Draw filter text
            if (isbufferEmpty && !isInputFocused)
            {
                ImGui::SetCursorPos(initialCursorPos + Float2{8, 0});
                ImGui::AlignTextToFramePadding();
				TextColorScope color = Style::s_colorTextDisabled;
				{
					ImGui::Label(m_filterHelpText);
				}
            }

            ::ImGui::PopStyleColor(1);
        }
        ::ImGui::EndChild();
        ::ImGui::PopStyleVar(2);
        ::ImGui::PopStyleColor();

        //-------------------------------------------------------------------------

        ::ImGui::PopID();
        return filterUpdated;
    }

    void FilterWidget::Clear()
    {
        Platform::MemoryClear(m_buffer.Get(), s_bufferSize);
        m_tokens.Clear();
    }

    bool FilterWidget::MatchesFilter(StringView string)
    {
        if (string.IsEmpty())
        {
            return false;
        }

        if (m_tokens.IsEmpty())
        {
            return true;
        }

        //-------------------------------------------------------------------------
        for (auto const &token : m_tokens)
        {
            if (string.Find(token.Get()) == INVALID_INDEX)
            {
                return false;
            }
        }

        //-------------------------------------------------------------------------

        return true;
    }

    void FilterWidget::SetFilter(String const &filterText)
    {
        uint32 const lengthToCopy = Math::Min(s_bufferSize, (uint32)filterText.Length());
		m_buffer.Set(filterText.Get(), lengthToCopy);
        OnBufferUpdated();
    }

    void FilterWidget::OnBufferUpdated()
    {
		m_buffer.Split(' ', m_tokens);

        for (auto &token : m_tokens)
        {
			token.ToLower();
        }
    }
}

#endif