#include "GUILayout.h"

#ifdef SE_DEVELOPMENT
#include "Runtime/Render/Assets/Texture/Texture.h"

#include "Core/Types/Collections/Sorting.h"
#include "Core/Types/Strings/StringConverter.h"
#include "GUIFont.h"

//-------------------------------------------------------------------------

namespace ImGui
{
    //-------------------------------------------------------------------------
    // Conversion
    //-------------------------------------------------------------------------

    ImTextureID ToIm(SE::GPUTexture const& texture )
    {
        return (ImU64) &texture;
    }

    ImTextureID ToIm(SE::GPUTexture const* pTexture )
    {
        return (ImU64) pTexture;
    }

    //-------------------------------------------------------------------------
    // General helpers
    //-------------------------------------------------------------------------

    bool BeginViewportPopupModal(SE::StringView popupName, bool* pIsPopupOpen, SE::Float2 const& size, ImGuiCond windowSizeCond, ImGuiWindowFlags windowFlags )
    {
		SE::StringAsANSI convert(popupName.Get(), popupName.Length());
        ImGui::OpenPopup(convert.Get());
        if ( size.x != 0 || size.y != 0 )
        {
            ImGui::SetNextWindowSize( size, windowSizeCond );
        }
//        ImGui::SetNextWindowViewport( ImGui::GetWindowViewport()->ID );
        return ImGui::BeginPopupModal(convert.Get(), pIsPopupOpen, windowFlags );
    }

    void MakeTabVisible( char const* const pWindowName )
    {
        ENGINE_ASSERT( pWindowName != nullptr );
/*        ImGuiWindow* pWindow = ImGui::FindWindowByName( pWindowName );
        if ( pWindow == nullptr || pWindow->DockNode == nullptr || pWindow->DockNode->TabBar == nullptr )
        {
            return;
        }

        pWindow->DockNode->TabBar->NextSelectedTabId = pWindow->ID;*/
        ImGui::SetWindowFocus( pWindowName );
    }

    SE::Float2 ClampToRect( ImRect const& rect, SE::Float2 const& inPoint )
    {
        ImVec2 clampedPos;
        clampedPos.x = SE::Math::Clamp( inPoint.x, rect.Min.x, rect.Max.x );
        clampedPos.y = SE::Math::Clamp( inPoint.y, rect.Min.y, rect.Max.y );
        return clampedPos;
    }

    SE::Float2 GetClosestPointOnRectBorder( ImRect const& rect, SE::Float2 const& inPoint )
    {
        ImVec2 const points[4] =
        {
            ImLineClosestPoint( rect.GetTL(), rect.GetTR(), inPoint ),
            ImLineClosestPoint( rect.GetBL(), rect.GetBR(), inPoint ),
            ImLineClosestPoint( rect.GetTL(), rect.GetBL(), inPoint ),
            ImLineClosestPoint( rect.GetTR(), rect.GetBR(), inPoint )
        };

        float distancesSq[4] =
        {
            ImLengthSqr( points[0] - inPoint ),
            ImLengthSqr( points[1] - inPoint ),
            ImLengthSqr( points[2] - inPoint ),
            ImLengthSqr( points[3] - inPoint )
        };

        // Get closest point
        float lowestDistance = FLT_MAX;
        int32_t closestPointIdx = -1;
        for ( auto i = 0; i < 4; i++ )
        {
            if ( distancesSq[i] < lowestDistance )
            {
                closestPointIdx = i;
                lowestDistance = distancesSq[i];
            }
        }

        ENGINE_ASSERT( closestPointIdx >= 0 && closestPointIdx < 4 );
        return points[closestPointIdx];
    }

    //-------------------------------------------------------------------------
    // Menu
    //-------------------------------------------------------------------------

    SE_API_RUNTIME bool BeginMenu(SE::StringView label, bool enabled)
    {
		SE::StringAsANSI convert(label.Get(), label.Length());
        return ImGui::BeginMenuEx(convert.Get(), NULL, enabled);
    }

    SE_API_RUNTIME bool MenuItem(SE::StringView label, SE::StringView shortcut, bool selected, bool enabled)
    {
		SE::StringAsANSI labelConvert(label.Get(), label.Length());
		SE::StringAsANSI shortcutConvert(label.Get(), label.Length());
        return ImGui::MenuItemEx(labelConvert.Get(), NULL, shortcutConvert.Get(), selected, enabled);
    }

    //-------------------------------------------------------------------------
    // Separators
    //-------------------------------------------------------------------------

    static void CenteredSeparator( float width )
    {
        ImGuiWindow* window = ImGui::GetCurrentWindow();
        if ( window->SkipItems )
        {
            return;
        }

        ImGuiContext& g = *GImGui;

        // Horizontal Separator
        float x1, x2;
        if ( window->DC.CurrentColumns == nullptr && ( width == 0 ) )
        {
            // Span whole window
            x1 = window->DC.CursorPos.x;
            x2 = x1 + window->Size.x;
        }
        else
        {
            // Start at the cursor
            x1 = window->DC.CursorPos.x;
            if ( width != 0 )
            {
                x2 = x1 + width;
            }
            else
            {
                x2 = window->ClipRect.Max.x;

                // Pad right side of columns (except the last one)
                if ( window->DC.CurrentColumns && ( window->DC.CurrentColumns->Current < window->DC.CurrentColumns->Count - 1 ) )
                {
                    x2 -= g.Style.ItemSpacing.x;
                }
            }
        }
        float y1 = window->DC.CursorPos.y + int( window->DC.CurrLineSize.y / 2.0f );
        float y2 = y1 + 1.0f;

        window->DC.CursorPos.x += width; //+ g.Style.ItemSpacing.x;
        x1 += window->DC.GroupOffset.x;

        const ImRect bb( ImVec2( x1, y1 ), ImVec2( x2, y2 ) );
        ImGui::ItemSize( ImVec2( 0.0f, 0.0f ) ); // NB: we don't provide our width so that it doesn't get feed back into AutoFit, we don't provide height to not alter layout.
        if ( !ImGui::ItemAdd( bb, NULL ) )
        {
            return;
        }

        window->DrawList->AddLine( bb.Min, ImVec2( bb.Max.x, bb.Min.y ), ImGui::GetColorU32( ImGuiCol_Separator ) );
    }



    void TextSeparator(SE::StringView text, float preWidth, float desiredWidth)
    {
		SE::StringAsANSI textConvert(text.Get(), text.Length());
        float const availableWidth = ImGui::GetContentRegionAvail().x;
        float const textWidth = ImGui::CalcTextSize(textConvert.Get()).x;
        float const totalWidth = SE::Math::Min( preWidth + textWidth + ( ImGui::GetStyle().ItemSpacing.x * 2 ), availableWidth );

        ImGuiWindow* window = ImGui::GetCurrentWindow();
        if ( window->DC.CurrLineSize.y == 0 )
        {
            window->DC.CurrLineSize.y = ImGui::GetTextLineHeight();
        }
        CenteredSeparator( preWidth );

        //-------------------------------------------------------------------------

        ImGui::SameLine();
        ImGui::Text(textConvert.Get());

        //-------------------------------------------------------------------------

        // If we have a total width specified, calculate the post separator width
        float const remainingWidth = ( desiredWidth != 0 ) ? desiredWidth - totalWidth :  availableWidth - totalWidth;
        if ( remainingWidth > 0 )
        {
            ImGui::SameLine();
            CenteredSeparator( remainingWidth );
        }
    }

    void SameLineSeparator( float width, SE::Color const& color )
    {
        SE::Color32 const separatorColor = ( color == SE::Colors::Transparent ) ? SE::Color( ImGui::GetStyleColorVec4( ImGuiCol_Separator ) ) : SE::Color( color );
        ImVec2 const seperatorSize( width <= 0 ? ( ImGui::GetStyle().ItemSpacing.x * 2 ) + 1 : width, ImGui::GetFrameHeight() );

        ImGui::SameLine( 0, 0 );

        ImVec2 const canvasPos = ImGui::GetCursorScreenPos();
        float const startPosX = canvasPos.x + SE::Math::Floor( seperatorSize.x / 2 );
        float const startPosY = canvasPos.y + 1;
        float const endPosY = startPosY + seperatorSize.y - 2;

        ImDrawList* pDrawList = ImGui::GetWindowDrawList();
        pDrawList->AddLine( ImVec2( startPosX, startPosY ), ImVec2( startPosX, endPosY ), separatorColor, 1 );

        ImGui::Dummy( seperatorSize );
        ImGui::SameLine( 0, 0 );
    }

    //-------------------------------------------------------------------------
    // Basic Widgets
    //-------------------------------------------------------------------------

    void ItemTooltip(SE::StringView text)
    {
        ImGui::PushStyleVar( ImGuiStyleVar_WindowPadding, ImVec2( 4, 4 ) );
        if ( ImGui::IsItemHovered() && GImGui->HoveredIdTimer > SE::GUI::Style::s_toolTipDelay )
        {
			SE::StringAsANSI textConvert(text.Get(), text.Length());
            ImGui::SetTooltip(textConvert.Get());
        }
        ImGui::PopStyleVar();
    }

    void ItemTooltipDelayed(float tooltipDelay, const char* text)
    {
        ENGINE_ASSERT(tooltipDelay > 0);
        if ( ImGui::IsItemHovered() && GImGui->HoveredIdTimer > tooltipDelay )
        {
            ImGui::SetTooltip(text);
        }
    }

	SE::Float2 CalcTextSize(SE::StringView text)
	{
		SE::StringAsANSI textConvert(text.Get(), text.Length());
		return ImGui::CalcTextSize(textConvert.Get());
	}


    void Label(SE::StringView text)
    {
        if (text.IsEmpty())
        {
            return;
        }
		SE::StringAsANSI textConvert(text.Get(), text.Length());
        ImGui::TextUnformatted(textConvert.Get());
    }

    void Label(SE::StringView text, SE::StringView tooltip)
    {
        if (text.IsEmpty())
        {
            return;
        }

		SE::StringAsANSI textConvert(text.Get(), text.Length());
        ImGui::TextUnformatted(textConvert.Get());
        if (!tooltip.IsEmpty() && ImGui::IsItemHovered() && GImGui->HoveredIdTimer > SE::GUI::Style::s_toolTipDelay)
        {
			SE::StringAsANSI tooltipConvert(tooltip.Get(), tooltip.Length());
            ImGui::PushStyleVar( ImGuiStyleVar_WindowPadding, ImVec2(4, 4));
            ImGui::SetTooltip(tooltipConvert.Get());
            ImGui::PopStyleVar();
        }

    }

    bool Button(SE::StringView label, SE::StringView tooltip, const SE::Float2 &size)
    {
		SE::StringAsANSI labelConvert(label.Get(), label.Length());
        bool state = ImGui::ButtonEx(labelConvert.Get(), size, ImGuiButtonFlags_None);
        if (!tooltip.IsEmpty())
        {
            ItemTooltip(tooltip);
        }
        
        return state;
    }

    bool Button(SE::StringView label, const SE::Float2 &size)
    {
        return Button(label, nullptr, size);
    }

    bool Checkbox(SE::StringView label, bool *pValue)
    {
        ImVec2 const newFramePadding( 2, 2 );
        float const offsetY = ImGui::GetStyle().FramePadding.y - newFramePadding.y;
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + offsetY );

        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, newFramePadding );
		SE::StringAsANSI labelConvert(label.Get(), label.Length());
        bool result = ImGui::Checkbox( labelConvert.Get(), pValue );
        ImGui::PopStyleVar();

        return result;
    }

    bool IconButton(const SE::StringView& icon, SE::StringView label, SE::Color const& iconColor, SE::Float2 const& buttonSize, bool shouldCenterContents )
    {
		SE::StringAsANSI iconConvert(icon.Get(), icon.Length());
        if (!icon.IsEmpty())
        {
            return ImGui::Button(iconConvert.Get(), buttonSize );
        }

        //-------------------------------------------------------------------------

        ImGuiContext& g = *GImGui;

        ImGuiWindow* pWindow = ImGui::GetCurrentWindow();
        if ( pWindow->SkipItems )
        {
            return false;
        }

        ImGuiStyle const& style = ImGui::GetStyle();

        // Calculate sizes
        //-------------------------------------------------------------------------
		SE::StringAsANSI labelConvert(label.Get(), label.Length());
        ImGuiID const ID = pWindow->GetID(labelConvert.Get());
        ImVec2 const iconSize = ImGui::CalcTextSize( iconConvert.Get(), nullptr, true );
        ImVec2 const labelSize = ImGui::CalcTextSize( labelConvert.Get(), nullptr, true );

        float totalButtonContentsWidth = labelSize.x + iconSize.x + style.ItemSpacing.x;

        if ( shouldCenterContents )
        {
            if ( labelSize.x > 0 )
            {
                totalButtonContentsWidth = labelSize.x + ( iconSize.x + style.ItemSpacing.x ) * 2;
            }
            else
            {
                totalButtonContentsWidth = iconSize.x;
            }
        }

        float totalButtonWidth = totalButtonContentsWidth + ( style.FramePadding.x * 2.0f );

        float const totalButtonHeight = SE::Math::Max( iconSize.y, labelSize.y ) + ( style.FramePadding.y * 2.0f );

        ImVec2 const pos = pWindow->DC.CursorPos;
        ImVec2 const finalButtonSize = ImGui::CalcItemSize( buttonSize, totalButtonWidth, totalButtonHeight );

        // Add item and handle input
        //-------------------------------------------------------------------------

        ImRect const bb( pos, pos + finalButtonSize );
        ImGui::ItemSize( finalButtonSize, style.FramePadding.y );
        if ( !ImGui::ItemAdd( bb, ID ) )
        {
            return false;
        }

        bool hovered, held;
        bool pressed = ImGui::ButtonBehavior( bb, ID, &hovered, &held, 0 );

        // Render Button
        //-------------------------------------------------------------------------

        // Render frame
        ImU32 const color = ImGui::GetColorU32( ( held && hovered ) ? ImGuiCol_ButtonActive : hovered ? ImGuiCol_ButtonHovered : ImGuiCol_Button );
//      ImGui::RenderNavHighlight( bb, ID );
        ImGui::RenderFrame( bb.Min, bb.Max, color, true, style.FrameRounding );

        bool const isDisabled = g.CurrentItemFlags & ImGuiItemFlags_Disabled;
        SE::Color32 const finalIconColor = isDisabled ? SE::GUI::Style::s_colorTextDisabled : iconColor;

        if ( shouldCenterContents )
        {
            // Icon and Label - ensure label is centered!
            if ( labelSize.x > 0 )
            {
                ImVec2 const textOffset( ( finalButtonSize.x - labelSize.x ) / 2.0f, style.FramePadding.y );
                ImGui::RenderTextClipped( bb.Min + textOffset, bb.Max - style.FramePadding, labelConvert.Get(), NULL, &labelSize, ImVec2( 0, 0.5f ), &bb );

                ImVec2 const iconOffset( textOffset.x - iconSize.x - style.ItemSpacing.x, style.FramePadding.y );
                pWindow->DrawList->AddText( pos + iconOffset, finalIconColor, labelConvert.Get() );
            }
            else // Only an icon
            {
                ImVec2 const iconOffset( ( finalButtonSize.x - iconSize.x ) / 2.0f, style.FramePadding.y );
                pWindow->DrawList->AddText( pos + iconOffset, finalIconColor, labelConvert.Get() );
            }
        }
        else // No centering
        {
            ImVec2 const textOffset( style.FramePadding.x + iconSize.x + style.ItemSpacing.x, style.FramePadding.y );
            ImGui::RenderTextClipped( bb.Min + textOffset, bb.Max - style.FramePadding, labelConvert.Get(), NULL, &labelSize, ImVec2( 0, 0.5f ), &bb );
            pWindow->DrawList->AddText( pos + style.FramePadding, finalIconColor, labelConvert.Get() );
        }

        return pressed;
    }

    bool ColoredButton( SE::Color const& backgroundColor, SE::Color const& foregroundColor, SE::StringView label, SE::Float2 const& size )
    {
        SE::Color const hoveredColor = backgroundColor * 1.15f;
        SE::Color const activeColor = backgroundColor * 1.25f;

        ImGui::PushStyleColor( ImGuiCol_Button, backgroundColor.ToFloat4() );
        ImGui::PushStyleColor( ImGuiCol_ButtonHovered, hoveredColor);
        ImGui::PushStyleColor( ImGuiCol_ButtonActive, activeColor);
        ImGui::PushStyleColor( ImGuiCol_Text, foregroundColor.ToFloat4() );
		SE::StringAsANSI labelConvert(label.Get(), label.Length());
        bool const result = ImGui::Button( labelConvert.Get(), size );
        ImGui::PopStyleColor( 4 );

        return result;
    }

    bool FlatButton(SE::StringView label, SE::Float2 const& size )
    {
        ImGui::PushStyleColor( ImGuiCol_Button, ImVec4( 0, 0, 0, 0 ) );
		SE::StringAsANSI labelConvert(label.Get(), label.Length());
        bool const result = ImGui::Button( labelConvert.Get(), size );
        ImGui::PopStyleColor( 1 );

        return result;
    }

    bool ColoredIconButton( SE::Color const& backgroundColor, SE::Color const& foregroundColor, SE::Color const& iconColor, SE::StringView icon, SE::StringView label, SE::Float2 const& size, bool shouldCenterContents )
    {
        SE::Color32 const hoveredColor = backgroundColor * 1.15f;
		SE::Color32 const activeColor = backgroundColor * 1.25f;

        ImGui::PushStyleColor( ImGuiCol_Button, backgroundColor );
        ImGui::PushStyleColor( ImGuiCol_ButtonHovered, hoveredColor );
        ImGui::PushStyleColor( ImGuiCol_ButtonActive, activeColor );
        ImGui::PushStyleColor( ImGuiCol_Text, foregroundColor );
        bool const result = IconButton( icon, label, iconColor, size, shouldCenterContents );
        ImGui::PopStyleColor( 4 );

        return result;
    }

    bool FlatIconButton( SE::StringView icon, SE::StringView label, SE::Color const& iconColor, SE::Float2 const& size, bool shouldCenterContents )
    {
        ImGui::PushStyleColor( ImGuiCol_Button, ImVec4( 0, 0, 0, 0 ) );
        bool const result = IconButton( icon, label, iconColor, size, shouldCenterContents );
        ImGui::PopStyleColor( 1 );

        return result;
    }

    bool IconButtonWithDropDown( char const* comboID, SE::StringView icon, SE::StringView buttonLabel, SE::Color const& iconColor, float buttonWidth,
            void (&comboCallback)(), bool shouldCenterContents )
    {
        SE::StringAnsi const comboIDStr = SE::StringAnsi::Format("##{0}", comboID);

        // Calculate button size
        //-------------------------------------------------------------------------

        constexpr float const comboWidth = 26;
        if ( buttonWidth > comboWidth )
        {
            buttonWidth -= comboWidth;
        }
        else if ( buttonWidth > 0 )
        {
            buttonWidth = 1;
        }
        else if ( buttonWidth < 0 )
        {
            buttonWidth = ImGui::GetContentRegionAvail().x - comboWidth;
        }

        // Button
        //-------------------------------------------------------------------------

        SE::Float2 const actualButtonSize = SE::Float2( buttonWidth, 0.0f );
        bool const buttonResult = IconButton( icon, buttonLabel, iconColor, actualButtonSize, shouldCenterContents );

        uint32 color = ImGui::GetColorU32( ImGuiCol_Button );
        if ( ImGui::IsItemHovered() )
        {
            if ( ImGui::IsItemActive() )
            {
                color = ImGui::GetColorU32( ImGuiCol_ButtonActive );
            }
            else
            {
                color = ImGui::GetColorU32( ImGuiCol_ButtonHovered );
            }
        }

        //-------------------------------------------------------------------------

        ImGui::SameLine( 0, 0 );
        SE::Float2 const cursorPos = ImGui::GetCursorPos();

        // Combo
        //-------------------------------------------------------------------------

        ImGui::SetNextItemWidth( comboWidth );
        if ( ImGui::BeginCombo( comboIDStr.Get(), nullptr, ImGuiComboFlags_NoPreview | ImGuiComboFlags_PopupAlignLeft | ImGuiComboFlags_HeightLargest ) )
        {
            ImGui::PushStyleVar( ImGuiStyleVar_CellPadding, SE::Float2( 4, 8 ) );
            ImGui::PushStyleColor( ImGuiCol_TableBorderStrong, 0 );
            ImGui::PushStyleColor( ImGuiCol_TableBorderLight, 0 );
            bool const drawTable = ImGui::BeginTable( "LayoutTable", 1, ImGuiTableFlags_Borders );
            ImGui::PopStyleVar();
            ImGui::PopStyleColor( 2 );

            if ( drawTable )
            {
                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                comboCallback();
                ImGui::EndTable();
            }

            ImGui::EndCombo();
        }

        auto pDrawList = ImGui::GetWindowDrawList();
        SE::Float2 const fillerMin = ImGui::GetWindowPos() + SE::Float2( cursorPos ) - SE::Float2( ImGui::GetStyle().FrameRounding, 0.0f );
        SE::Float2 const fillerMax = ImGui::GetWindowPos() + SE::Float2( cursorPos ) + SE::Float2( ImGui::GetStyle().FrameRounding, ImGui::GetFrameHeight() );
        pDrawList->AddRectFilled( fillerMin, fillerMax, color );

        // Fill Gap
        //-------------------------------------------------------------------------

        return buttonResult;
    }

    bool ToggleButton( char const* pOnLabel, char const* pOffLabel, bool& value, SE::Float2 const& size, SE::Color  const& onColor, SE::Color const& offColor )
    {
        ImGui::PushStyleColor( ImGuiCol_Text, SE::Color32( value ? onColor : offColor ) );
        bool const result = ImGui::Button( value ? pOnLabel : pOffLabel, size );
        ImGui::PopStyleColor();

        //-------------------------------------------------------------------------

        if ( result )
        {
            value = !value;
        }

        return result;
    }

    bool FlatToggleButton( char const* pOnLabel, char const* pOffLabel, bool& value, SE::Float2 const& size, SE::Color const& onColor, SE::Color const& offColor )
    {
        ImGui::PushStyleColor( ImGuiCol_Button, ImVec4( 0, 0, 0, 0 ) );
        bool result = ToggleButton( pOnLabel, pOffLabel, value, size, onColor, offColor );
        ImGui::PopStyleColor( 1 );

        return result;
    }

    void DrawArrow( ImDrawList* pDrawList, SE::Float2 const& arrowStart, SE::Float2 const& arrowEnd, SE::Color const& color, float arrowWidth, float arrowHeadWidth )
    {
        ENGINE_ASSERT( pDrawList != nullptr );

        ImVec2 const direction = SE::Float2::Normalize( arrowEnd - arrowStart );
        ImVec2 const orthogonalDirection( -direction.y, direction.x );

        ImVec2 const triangleSideOffset = orthogonalDirection * arrowHeadWidth;
        ImVec2 const triBase = (ImVec2)arrowEnd - ( direction * arrowHeadWidth );
        ImVec2 const tri1 = triBase - triangleSideOffset;
        ImVec2 const tri2 = triBase + triangleSideOffset;

		SE::Color32 color32 = color;
        pDrawList->AddLine( arrowStart, triBase, color32, arrowWidth );
        pDrawList->AddTriangleFilled( arrowEnd, tri1, tri2, color32 );
    }

    bool DrawOverlayIcon( ImVec2 const& iconPos, char icon[4], void* iconID, bool isSelected, SE::Color const& selectedColor )
    {
        bool result = false;

        //-------------------------------------------------------------------------

        SE::GUI::ScopedFont scopedFont( SE::GUI::Font::Large );
        ImVec2 const textSize = ImGui::CalcTextSize( icon );
        ImVec2 const iconSize = textSize + ( ImGui::GetStyle().FramePadding * 2 );
        ImVec2 const iconHalfSize( iconSize.x / 2, iconSize.y / 2 );
        ImRect const iconRect( iconPos - iconHalfSize, iconPos + iconHalfSize );
        ImRect const windowRect( ImVec2( 0, 0 ), ImGui::GetWindowSize() );
        if ( !windowRect.Overlaps( iconRect ) )
        {
            return result;
        }

        //-------------------------------------------------------------------------

        SE::Color32 iconColor = SE::GUI::Style::s_colorText;
        if ( isSelected || iconRect.Contains( ImGui::GetMousePos() - ImGui::GetWindowPos() ) )
        {
            iconColor = selectedColor;
        }

        //-------------------------------------------------------------------------

        ImGui::SetCursorPos( iconPos - iconHalfSize );
        ImGui::PushID( iconID );
        ImGui::PushStyleColor( ImGuiCol_Button, ImVec4( 0, 0, 0, 0 ) );
        ImGui::PushStyleColor( ImGuiCol_ButtonActive, ImVec4( 0, 0, 0, 0 ) );
        ImGui::PushStyleColor( ImGuiCol_ButtonHovered, ImVec4( 0, 0, 0, 0 ) );
        ImGui::PushStyleColor( ImGuiCol_Text, iconColor );
        if ( ImGui::Button( icon, iconSize ) && !isSelected )
        {
            result = true;
        }
        ImGui::PopStyleColor( 4 );
        ImGui::PopID();

        return result;
    }

    bool DrawSpinner( char const* pLabel, SE::Color const& color, SE::Float2 size, float thickness, float padding )
    {
        static float const numSegments = 30.0f;

        //-------------------------------------------------------------------------

        ImGuiWindow* pWindow = ImGui::GetCurrentWindow();
        if ( pWindow->SkipItems )
        {
            return false;
        }

        //-------------------------------------------------------------------------

        ImGuiStyle const& style = ImGui::GetStyle();

        // Calculate final size
        //-------------------------------------------------------------------------

        if ( size.x == 0 )
        {
            size.x = ImGui::GetFrameHeight();
        }
        else if ( size.x <= 0 )
        {
            size.x = ImGui::GetContentRegionAvail().x;
        }

        if ( size.y == 0 )
        {
            size.y = ImGui::GetFrameHeight();
        }
        else if ( size.y <= 0 )
        {
            size.y = ImGui::GetContentRegionAvail().y;
        }

        if ( size.x < 0 || size.y < 0 )
        {
            return false;
        }

        // Calculate pos, radius and bounding box
        //-------------------------------------------------------------------------

        ImVec2 const pos = pWindow->DC.CursorPos;
        float const radius = ( ( SE::Math::Min( size.x, size.y ) - thickness ) / 2 ) - padding;
        ImRect const bb( pos, ImVec2( pos.x + size.x, pos.y + size.y ) );

        // Add invisible button
        //-------------------------------------------------------------------------

        bool buttonResult = ImGui::InvisibleButton( pLabel, size );

        // Draw
        //-------------------------------------------------------------------------

        // Debug Only
        {
            //pWindow->DrawList->AddRect( bb.Min, bb.Max, SE::Colors::Pink.ToUInt32() );
        }

        pWindow->DrawList->PathClear();

        float const time = (float) ImGui::GetTime();
        float const start = SE::Math::Abs( SE::Math::Sin( time * 1.8f ) * ( numSegments - 5 ) );
        float const min = SE::Math::PI * 2.0f * ( start ) / numSegments;
        float const max = SE::Math::PI * 2.0f * ( numSegments - 3 ) / numSegments;

        ImVec2 const center = ImVec2( pos.x + radius + padding + ( thickness / 2 ), pos.y + radius + padding + ( thickness / 2 ) );

        for ( float i = 0; i < numSegments; i++ )
        {
            float const a = min + ( i / numSegments ) * ( max - min );
            float const b = a + ( time * 8 );
            pWindow->DrawList->PathLineTo( ImVec2( center.x + SE::Math::Cos( b ) * radius, center.y + SE::Math::Sin( b ) * radius ) );
        }

        pWindow->DrawList->PathStroke(SE::Color32(color), false, thickness );

        return buttonResult;
    }

    //-------------------------------------------------------------------------

    constexpr static float const g_labelWidth = 20.0f;

    static bool BeginElementFrame( char const* pLabel, float labelWidth, SE::Float2 const& size, SE::Color const& backgroundColor)
    {
        ImGui::PushStyleVar( ImGuiStyleVar_ChildRounding, 0.0f );
        ImGui::PushStyleVar( ImGuiStyleVar_ChildBorderSize, 0.0f );
        ImGui::PushStyleVar( ImGuiStyleVar_WindowPadding, ImVec2( 0, 0 ) );


        if (ImGui::BeginChild(pLabel, size, false, ImGuiWindowFlags_AlwaysUseWindowPadding | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse ))
        {
            ImGui::AlignTextToFramePadding();
            ImGui::SetCursorPosX( 3 );
            ImGui::PushStyleColor( ImGuiCol_Text, backgroundColor.ToFloat4());
            ImGui::Text( pLabel );
            ImGui::PopStyleColor();
            ImGui::SameLine( 0, 0 );
            ImGui::SetCursorPosX( labelWidth );
            ImGui::SetCursorPosY( ImGui::GetCursorPosY() + 1 );

            return true;
        }


        ImGui::PopStyleVar(3);
        return false;
    }

    static void EndElementFrame()
    {
        ImGui::EndChild();
        ImGui::PopStyleVar( 3 );
    }

    static bool DrawFloatElement( char const* pID, char const* pLabel, float const& width, SE::Color const& backgroundColor, float* pValue, bool isReadOnly = false )
    {
        bool result = false;

        SE::GUI::ScopedFont sf( SE::GUI::Font::Small );

        if ( BeginElementFrame( pLabel, g_labelWidth, SE::Float2( width, ImGui::GetFrameHeight() ), backgroundColor ) )
        {
            ImGui::SetNextItemWidth( width - g_labelWidth - 1 );
            bool change = ImGui::DragFloat( pID, pValue, 0.1f, 0, 0, "%.3f", isReadOnly ? ImGuiInputTextFlags_ReadOnly : 0 );
            result = ImGui::IsItemDeactivatedAfterEdit() || change;

            EndElementFrame();
        }

        return result;
    }

    //-------------------------------------------------------------------------

    bool InputFloat2( char const* pID, SE::Float2& value, float width, bool isReadOnly )
    {
        float const contentWidth = ( width > 0 ) ? width : ImGui::GetContentRegionAvail().x;
        float const itemSpacing = ImGui::GetStyle().ItemSpacing.x;
        float const inputWidth = SE::Math::Floor( ( contentWidth - itemSpacing ) / 2 );

        //-------------------------------------------------------------------------

        bool valueUpdated = false;

        ImGui::PushID( pID );
        {
            if ( DrawFloatElement( "##x", "X", inputWidth, SE::Colors::MediumRed, &value.x, isReadOnly ) )
            {
                valueUpdated = true;
            }

            ImGui::SameLine( 0, itemSpacing );
            if ( DrawFloatElement( "##y", "Y", inputWidth, SE::Colors::LimeGreen, &value.y, isReadOnly ) )
            {
                valueUpdated = true;
            }
        }
        ImGui::PopID();

        return valueUpdated;
    }

    bool InputFloat3( char const* pID, SE::Float3& value, float width, bool isReadOnly )
    {
        float const contentWidth = ( width > 0 ) ? width : ImGui::GetContentRegionAvail().x;
        float const itemSpacing = ImGui::GetStyle().ItemSpacing.x;
        float const inputWidth = SE::Math::Floor( ( contentWidth - ( itemSpacing * 2 ) ) / 3 );

        //-------------------------------------------------------------------------

        bool valueUpdated = false;

        ImGui::PushID( pID );
        {
            if ( DrawFloatElement( "##x", "X", inputWidth, SE::Colors::White, &value.x, isReadOnly ) )
            {
                valueUpdated = true;
            }

            ImGui::SameLine( 0, itemSpacing );
            if ( DrawFloatElement( "##y", "Y", inputWidth, SE::Colors::White, &value.y, isReadOnly ) )
            {
                valueUpdated = true;
            }

            ImGui::SameLine( 0, itemSpacing );
            if ( DrawFloatElement( "##z", "Z", inputWidth, SE::Colors::White, &value.z, isReadOnly ) )
            {
                valueUpdated = true;
            }
        }
        ImGui::PopID();

        return valueUpdated;
    }

    bool InputFloat4( char const* pID, SE::Float4& value, float width, bool isReadOnly )
    {
        float const contentWidth = ( width > 0 ) ? width : ImGui::GetContentRegionAvail().x;
        float const itemSpacing = ImGui::GetStyle().ItemSpacing.x;
        float const inputWidth = SE::Math::Floor( ( contentWidth - ( itemSpacing * 3 ) ) / 4 );

        //-------------------------------------------------------------------------

        bool valueUpdated = false;

        ImGui::PushID( pID );
        {
            if ( DrawFloatElement( "##x", "X", inputWidth, SE::Colors::MediumRed, &value.x, isReadOnly ) )
            {
                valueUpdated = true;
            }

            ImGui::SameLine( 0, itemSpacing );
            if ( DrawFloatElement( "##y", "Y", inputWidth, SE::Colors::LimeGreen, &value.y, isReadOnly ) )
            {
                valueUpdated = true;
            }

            ImGui::SameLine( 0, itemSpacing );
            if ( DrawFloatElement( "##z", "Z", inputWidth, SE::Colors::RoyalBlue, &value.z, isReadOnly ) )
            {
                valueUpdated = true;
            }

            ImGui::SameLine( 0, itemSpacing );
            if ( DrawFloatElement( "##w", "W", inputWidth, SE::Colors::DarkOrange, &value.w, isReadOnly ) )
            {
                valueUpdated = true;
            }
        }
        ImGui::PopID();

        return valueUpdated;
    }

    bool InputTransform(SE::Transform& value, float width, bool readOnly )
    {
        bool valueUpdated = false;

        ImGui::PushStyleVar( ImGuiStyleVar_CellPadding, ImVec2( 0, 2 ) );
        if ( ImGui::BeginTable( "Transform", 2, ImGuiTableFlags_None, ImVec2( width, 0 ) ) )
        {
            ImGui::TableSetupColumn( "Header", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize, 55 );
            ImGui::TableSetupColumn( "Values", ImGuiTableColumnFlags_NoHide | ImGuiTableColumnFlags_WidthStretch );

            ImGui::TableNextRow();
            {
                ImGui::TableNextColumn();
                ImGui::AlignTextToFramePadding();
                ImGui::Text( "Position" );

                ImGui::TableNextColumn();
                SE::Float3 translation = value.Translation;
                if ( InputFloat3( "T", translation) )
                {
                    value.Translation = translation;
                    valueUpdated = true;
                }
            }

            ImGui::TableNextRow();
            {
                ImGui::TableNextColumn();
                ImGui::AlignTextToFramePadding();
                ImGui::Text( "Rotate" );

                ImGui::TableNextColumn();
                SE::Float3 rotation = value.Orientation.GetEuler();
                if ( InputFloat3( "R", rotation ) )
                {
                    value.Orientation = SE::Quaternion::Euler(rotation);
                    valueUpdated = true;
                }
            }

            ImGui::TableNextRow();
            {
                ImGui::TableNextColumn();
                ImGui::AlignTextToFramePadding();
                ImGui::Text( "Scale" );

                ImGui::TableNextColumn();
                SE::Float3 scale = value.Scale;
                ImGui::SetNextItemWidth( -1 );
                if ( InputFloat3( "##S", scale) )
                {
                    value.Scale = scale;
                    valueUpdated = true;
                }
            }

            ImGui::EndTable();
        }
        ImGui::PopStyleVar();

        return valueUpdated;
    }

	struct InputTextCallback_UserData
	{
		SE::String*            Str;
		ImGuiInputTextCallback  ChainCallback;
		void*                   ChainCallbackUserData;
	};

	static int InputTextCallback(ImGuiInputTextCallbackData* data)
	{
		InputTextCallback_UserData* user_data = (InputTextCallback_UserData*)data->UserData;
		if (data->EventFlag == ImGuiInputTextFlags_CallbackResize)
		{
			// Resize string callback
			// If for some reason we refuse the new length (BufTextLen) and/or capacity (BufSize) we need to set them back to what we want.
			SE::String* str = user_data->Str;
			ENGINE_ASSERT(data->Buf == (char*)str->Get());
			str->Resize(data->BufTextLen * sizeof(SE::Char));
			data->Buf = (char*)str->Get();
		}
		else if (user_data->ChainCallback)
		{
			// Forward to user callback, if any
			data->UserData = user_data->ChainCallbackUserData;
			return user_data->ChainCallback(data);
		}
		return 0;
	}

	bool InputText(const char* label, SE::String& str, ImGuiInputTextFlags flags, void* user_data)
	{
		ENGINE_ASSERT((flags & ImGuiInputTextFlags_CallbackResize) == 0);
		flags |= ImGuiInputTextFlags_CallbackResize;

		InputTextCallback_UserData cb_user_data;
		cb_user_data.Str = &str;
		cb_user_data.ChainCallback = nullptr;
		cb_user_data.ChainCallbackUserData = user_data;
		return ImGui::InputText(label, (char*)str.Get(), str.Length() + 1, flags, InputTextCallback, &cb_user_data);
	}

	bool InputTextMultiline(const char* label, SE::String& str, const SE::Float2& size, ImGuiInputTextFlags flags, void* user_data)
	{
		ENGINE_ASSERT((flags & ImGuiInputTextFlags_CallbackResize) == 0);
		flags |= ImGuiInputTextFlags_CallbackResize;

		InputTextCallback_UserData cb_user_data;
		cb_user_data.Str = &str;
		cb_user_data.ChainCallback = nullptr;
		cb_user_data.ChainCallbackUserData = user_data;
		return ImGui::InputTextMultiline(label, (char*)str.Get(), str.Length() + 1, size, flags, InputTextCallback, &cb_user_data);
	}

	bool InputTextWithHint(const char* label, const char* hint, SE::String& str, ImGuiInputTextFlags flags, void* user_data)
	{
		ENGINE_ASSERT((flags & ImGuiInputTextFlags_CallbackResize) == 0);
		flags |= ImGuiInputTextFlags_CallbackResize;

		InputTextCallback_UserData cb_user_data;
		cb_user_data.Str = &str;
		cb_user_data.ChainCallback = nullptr;
		cb_user_data.ChainCallbackUserData = user_data;
		return ImGui::InputTextWithHint(label, hint, (char*)str.Get(), str.Length() + 1, flags, InputTextCallback, &cb_user_data);
	}

	//-------------------------------------------------------------------------

	bool BeginCombo(const SE::StringView label, const SE::StringView preview_value, ImGuiComboFlags flags)
	{
		SE::StringAsUTF8 textConvert(label.Get(), label.Length());
		SE::StringAsUTF8 viewConvert(preview_value.Get(), preview_value.Length());
		return ImGui::BeginCombo(textConvert.Get(), viewConvert.Get(), flags);
	}


	bool Selectable(const SE::StringView label, bool selected, ImGuiSelectableFlags flags, const SE::Float2& size)
	{
		SE::StringAsUTF8 labelConvert(label.Get(), label.Length());
		return ImGui::Selectable(labelConvert.Get(), selected, flags, size);
	}

	bool Selectable(const SE::StringView label, bool* p_selected, ImGuiSelectableFlags flags, const SE::Float2& size)
	{
		SE::StringAsUTF8 labelConvert(label.Get(), label.Length());
		return ImGui::Selectable(labelConvert.Get(), p_selected, flags, size);
	}


	//-------------------------------------------------------------------------

    void Image(SE::Texture* img, SE::Float2 size, SE::Float2 const &uv0, SE::Float2 const &uv1, SE::Color const &tintColor, SE::Color const &borderColor)
    {
        ImGui::Image((ImTextureID)img->GetTexture(), size, uv0, uv1, tintColor, borderColor);
    }

    void Image(SE::GPUTexture* img, SE::Float2 size, SE::Float2 const &uv0, SE::Float2 const &uv1, SE::Color const &tintColor, SE::Color const &borderColor)
	{
	    ImGui::Image((ImTextureID)img, size, uv0, uv1, tintColor, borderColor);
	}

/*    void ImageButton(char const *pButtonID, ImageInfo const &img, SE::Float2 const &uv0, SE::Float2 const &uv1, SE::Color const &backgroundColor, SE::Color const &tintColor)
    {
        ImGui::ImageButton(pButtonID, img.id, img.size, uv0, uv1, backgroundColor, tintColor);
    }*/


    //-------------------------------------------------------------------------

    void OrientationGuide::Draw(SE::Float2 const& guideOrigin, SE::Viewport const& viewport )
    {
        // Project world space axis positions to screen space
        //-------------------------------------------------------------------------
/*
        VectorSIMD const& originWS = viewport.GetViewPosition() + viewport.GetViewForwardDirection() * g_worldRenderDistanceZ;
        VectorSIMD const& worldAxisX = ( VectorSIMD::UnitX );
        VectorSIMD const& worldAxisY = ( VectorSIMD::UnitY );
        VectorSIMD const& worldAxisZ = ( VectorSIMD::UnitZ );

        VectorSIMD const& worldAxisForwardPointX = ( originWS + worldAxisX );
        VectorSIMD const& worldAxisForwardPointY = ( originWS + worldAxisY );
        VectorSIMD const& worldAxisForwardPointZ = ( originWS + worldAxisZ );
        VectorSIMD const& worldAxisBackwardPointX = ( originWS - worldAxisX );
        VectorSIMD const& worldAxisBackwardPointY = ( originWS - worldAxisY );
        VectorSIMD const& worldAxisBackwardPointZ = ( originWS - worldAxisZ );

        VectorSIMD const axisStartPointX = VectorSIMD( viewport.WorldSpaceToScreenSpace( worldAxisBackwardPointX ) );
        VectorSIMD const axisStartPointY = VectorSIMD( viewport.WorldSpaceToScreenSpace( worldAxisBackwardPointY ) );
        VectorSIMD const axisStartPointZ = VectorSIMD( viewport.WorldSpaceToScreenSpace( worldAxisBackwardPointZ ) );
        VectorSIMD const axisEndPointX = VectorSIMD( viewport.WorldSpaceToScreenSpace( worldAxisForwardPointX ) );
        VectorSIMD const axisEndPointY = VectorSIMD( viewport.WorldSpaceToScreenSpace( worldAxisForwardPointY ) );
        VectorSIMD const axisEndPointZ = VectorSIMD( viewport.WorldSpaceToScreenSpace( worldAxisForwardPointZ ) );

        // Calculate screen space axis lengths
        //-------------------------------------------------------------------------

        float const axisLengthX = axisStartPointX.GetDistance2( axisEndPointX );
        float const axisLengthY = axisStartPointY.GetDistance2( axisEndPointY );
        float const axisLengthZ = axisStartPointZ.GetDistance2( axisEndPointZ );
        float const maxAxisLength = SE::Math::Max( axisLengthX, SE::Math::Max( axisLengthY, axisLengthZ ) );

        static float const axisHalfLengthSS = g_axisHalfLength;
        float const axisScaleX = ( axisLengthX / maxAxisLength ) * axisHalfLengthSS;
        float const axisScaleY = ( axisLengthY / maxAxisLength ) * axisHalfLengthSS;
        float const axisScaleZ = ( axisLengthZ / maxAxisLength ) * axisHalfLengthSS;

        // Calculate screen space axis directions
        VectorSIMD const origin = viewport.WorldSpaceToScreenSpace( originWS );
        VectorSIMD const axisDirX = ( axisEndPointX - origin ).GetNormalized2();
        VectorSIMD const axisDirY = ( axisEndPointY - origin ).GetNormalized2();
        VectorSIMD const axisDirZ = ( axisEndPointZ - origin ).GetNormalized2();

        // Sort SS axis and draw them in the correct order
        //-------------------------------------------------------------------------

        struct AxisDrawRequest { Axis m_axis; bool m_isInForwardDirection; float m_distance; };
        List<AxisDrawRequest, InlinedAllocation<6>> drawRequests;

        Plane const nearPlane = viewport.GetViewVolume().GetViewPlane(Render::ViewVolume::PlaneID::Near);

        drawRequests.Add( { Axis::X, true, nearPlane.GetAbsoluteDistanceToPoint( worldAxisForwardPointX ) } );
        drawRequests.Add( { Axis::Y, true, nearPlane.GetAbsoluteDistanceToPoint( worldAxisForwardPointY ) } );
        drawRequests.Add( { Axis::Z, true, nearPlane.GetAbsoluteDistanceToPoint( worldAxisForwardPointZ ) } );
        drawRequests.Add( { Axis::X, false, nearPlane.GetAbsoluteDistanceToPoint( worldAxisBackwardPointX ) } );
        drawRequests.Add( { Axis::Y, false, nearPlane.GetAbsoluteDistanceToPoint( worldAxisBackwardPointY ) } );
        drawRequests.Add( { Axis::Z, false, nearPlane.GetAbsoluteDistanceToPoint( worldAxisBackwardPointZ ) } );

		Function<bool(AxisDrawRequest const&, AxisDrawRequest const&)> compare = [] ( AxisDrawRequest const& lhs, AxisDrawRequest const& rhs ) {
		  return lhs.m_distance > rhs.m_distance;
		};
		Sorting::QuickSort(drawRequests, compare);

        //-------------------------------------------------------------------------

        auto pDrawList = ImGui::GetWindowDrawList();
        for ( auto const& request : drawRequests )
        {
            // X
            if ( request.m_axis == Axis::X && request.m_isInForwardDirection )
            {
                pDrawList->AddLine( guideOrigin, guideOrigin + axisDirX * ( axisScaleX - g_axisHeadRadius + 1.0f ), 0xBB0000FF, g_axisThickness );
                pDrawList->AddCircleFilled( guideOrigin + axisDirX * axisScaleX, g_axisHeadRadius, 0xBB0000FF );
            }
            else if ( request.m_axis == Axis::X && !request.m_isInForwardDirection )
            {
                pDrawList->AddCircleFilled( guideOrigin - axisDirX * axisScaleX, g_axisHeadRadius, 0x660000FF );
            }
            //Y
            else if ( request.m_axis == Axis::Y && request.m_isInForwardDirection )
            {
                pDrawList->AddLine( guideOrigin, guideOrigin + axisDirY * ( axisScaleY - g_axisHeadRadius + 1.0f ), 0xBB00FF00, g_axisThickness );
                pDrawList->AddCircleFilled( guideOrigin + axisDirY * axisScaleY, g_axisHeadRadius, 0xBB00FF00 );
            }
            else if ( request.m_axis == Axis::Y && !request.m_isInForwardDirection )
            {
                pDrawList->AddCircleFilled( guideOrigin - axisDirY * axisScaleY, g_axisHeadRadius, 0x6600FF00 );
            }
            // Z
            else if ( request.m_axis == Axis::Z && request.m_isInForwardDirection )
            {
                pDrawList->AddLine( guideOrigin, guideOrigin + axisDirZ * ( axisScaleZ - g_axisHeadRadius + 1.0f ), 0xBBFF0000, g_axisThickness );
                pDrawList->AddCircleFilled( guideOrigin + axisDirZ * axisScaleZ, g_axisHeadRadius, 0xBBFF0000 );
            }
            else if ( request.m_axis == Axis::Z && !request.m_isInForwardDirection )
            {
                pDrawList->AddCircleFilled( guideOrigin - axisDirZ * axisScaleZ, g_axisHeadRadius, 0x66FF0000 );
            }
        }*/
    }

    //-------------------------------------------------------------------------
    // Tree Node
    //-------------------------------------------------------------------------

    bool TreeNode(const SE::StringView label, ImGuiTreeNodeFlags flags)
    {
	    SE::StringAsUTF8 labelConvert(label.Get(), label.Length());
	    return ImGui::TreeNodeEx(labelConvert.Get(), flags);
    }

    void TreePush(SE::StringView str_id)
    {
	    SE::StringAsUTF8 labelConvert(str_id.Get(), str_id.Length());
	    ImGui::TreePush(labelConvert.Get());
    }

    bool TreeNode(uint32 id, SE::StringView label, ImGuiTreeNodeFlags flags)
    {
	    ImGuiWindow* window = GetCurrentWindow();
	    if (window->SkipItems)
	        return false;

	    SE::StringAsUTF8 labelConvert(label.Get(), label.Length());
	    return TreeNodeBehavior(id, flags, labelConvert.Get(), NULL);
    }

    //-------------------------------------------------------------------------
	// Docking and Window
	//-------------------------------------------------------------------------

	void DockingBuilderWindow(SE::StringView window_name, ImGuiID node_id)
	{
		SE::StringAsANSI labelConvert(window_name.Get(), window_name.Length());
		ImGui::DockBuilderDockWindow(labelConvert.Get(), node_id);
	}

	ImGuiWindow* FindWindowByName(SE::StringView window_name)
	{
		SE::StringAsANSI labelConvert(window_name.Get(), window_name.Length());
		return ImGui::FindWindowByName(labelConvert.Get());
	}

	bool Begin(SE::StringView name, bool* p_open, ImGuiWindowFlags flags)
	{
		SE::StringAsANSI labelConvert(name.Get(), name.Length());
		return ImGui::Begin(labelConvert.Get(), p_open, flags);
	}

}
#endif