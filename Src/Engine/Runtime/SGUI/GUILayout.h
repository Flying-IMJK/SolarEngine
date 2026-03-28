#pragma once

#include "Runtime/API.h"
#include "GUIFont.h"
#include "GUIStyle.h"

#include "Core/Math/Transform.h"
#include "Core/Math/Rectangle.h"
#include "Core/Math/Color.h"
#include "Core/Types/Strings/String.h"
#include "Core/Types/BitFlags.h"
#include "Core/Types/Delegate.h"
#include "Core/Types/Collections/List.h"

#include "Imgui/imgui_internal.h"
#include "Runtime/Graphics/Textures/GPUTexture.h"
#include "Runtime/Graphics/Viewport.h"
#include "Runtime/Resource/AssetRef.h"

//-------------------------------------------------------------------------
// ImGui Extensions
//-------------------------------------------------------------------------
// This is the primary integration of DearImgui in Esoterica.
//
// * Provides the necessary imgui state updates through the frame start/end functions
// * Provides helpers for common operations
//-------------------------------------------------------------------------

namespace SE
{
	class Texture;
}

namespace ImGui
{
    struct ImageInfo;

    //-------------------------------------------------------------------------
    // General helpers
    //-------------------------------------------------------------------------

    SE_API_RUNTIME void MakeTabVisible(char const *const pWindowName);

    SE_API_RUNTIME SE::Float2 ClampToRect(ImRect const &rect, SE::Float2 const &inPoint);

    // Returns the closest point on the rect border to the specified point
    SE_API_RUNTIME SE::Float2 GetClosestPointOnRectBorder(ImRect const &rect, SE::Float2 const &inPoint);

    // Is this a valid name ID character (i.e. A-Z, a-z, 0-9, _ )
    inline bool IsValidNameIDChar(ImWchar c)
    {
        return isalnum(c) || c == '_';
    }

    // Filter a text callback restricting it to valid name ID characters
    inline int FilterNameIDChars(ImGuiInputTextCallbackData *data)
    {
        if (IsValidNameIDChar(data->EventChar))
        {
            return 0;
        }
        return 1;
    }

    // Display a modal popup that is restricted to the current window's viewport
    SE_API_RUNTIME bool BeginViewportPopupModal(SE::StringView popupName, bool *pIsPopupOpen, SE::Float2 const &size = SE::Float2(0, 0), ImGuiCond windowSizeCond = ImGuiCond_Always, ImGuiWindowFlags windowFlags = (ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize));

    // Cancels an option dialog via ESC
    inline bool CancelDialogViaEsc(bool isDialogOpen)
    {
        if (::ImGui::IsKeyPressed(ImGuiKey_Escape))
        {
            ::ImGui::CloseCurrentPopup();
            return false;
        }

        return isDialogOpen;
    }

    //-------------------------------------------------------------------------
    // Menu
    //-------------------------------------------------------------------------
    SE_API_RUNTIME bool BeginMenu(SE::StringView label, bool enabled = true);

    SE_API_RUNTIME bool MenuItem(SE::StringView label, SE::StringView shortcut = nullptr, bool selected = false, bool enabled = true);


    //-------------------------------------------------------------------------
    // Separators
    //-------------------------------------------------------------------------

    // Create a labeled separator: --- TEXT ---------------
    SE_API_RUNTIME void TextSeparator(SE::StringView text, float preWidth = 10.0f, float desiredWidth = 0);

    // Same as the Imgui::SameLine except it also draws a vertical separator.
    SE_API_RUNTIME void SameLineSeparator(float width = 0, SE::Color const &color = SE::Colors::Transparent);

    //-------------------------------------------------------------------------
    // Basic Widgets
    //-------------------------------------------------------------------------

    // Draw a tooltip for the immediately preceding item
    SE_API_RUNTIME void ItemTooltip(SE::StringView text);

    // Draw a tooltip with a custom hover delay for the immediately preceding item
    SE_API_RUNTIME void ItemTooltipDelayed(float tooltipDelay, const char *text);

	SE_API_RUNTIME SE::Float2 CalcTextSize(SE::StringView text);

    SE_API_RUNTIME void Label(SE::StringView text);

    SE_API_RUNTIME void Label(SE::StringView text, SE::StringView tooltip);

	SE_API_RUNTIME void Label(SE::StringView text, SE::StringView tooltip);

    SE_API_RUNTIME bool Button(SE::StringView label, SE::StringView tooltip, const SE::Float2& size = SE::Float2(0, 0));

    SE_API_RUNTIME bool Button(SE::StringView label, const SE::Float2& size = SE::Float2(0, 0));

    // A smaller check box allowing us to use a larger frame padding value
    SE_API_RUNTIME bool Checkbox(SE::StringView label, bool *pValue);

    // Draw a button with an explicit icon
    SE_API_RUNTIME bool IconButton(const SE::StringView& icon, SE::StringView label, SE::Color const &iconColor = SE::Color(::ImGui::GetStyle().Colors[ImGuiCol_Text]), SE::Float2 const &size = SE::Float2(0, 0), bool shouldCenterContents = false);

    // Draw a colored button
    SE_API_RUNTIME bool ColoredButton(SE::Color const &backgroundColor, SE::Color const &foregroundColor, SE::StringView label, SE::Float2 const &size = SE::Float2(0, 0));

    // Draw a colored icon button
    SE_API_RUNTIME bool ColoredIconButton(SE::Color const &backgroundColor, SE::Color const &foregroundColor, SE::Color const &iconColor, SE::StringView icon, SE::StringView label, SE::Float2 const &size = SE::Float2(0, 0), bool shouldCenterContents = false);

    // Draws a flat button - a button with no background
    SE_API_RUNTIME bool FlatButton(SE::StringView label, SE::Float2 const &size = SE::Float2(0, 0));

    // Draws a flat button - with a custom text SE::Color
    inline bool FlatButtonColored(SE::Color const &foregroundColor, char const *label, SE::Float2 const &size = SE::Float2(0, 0))
    {
        ::ImGui::PushStyleColor(ImGuiCol_Button, 0);
        ::ImGui::PushStyleColor(ImGuiCol_Text, foregroundColor.ToFloat4());
        bool const result = ::ImGui::Button(label, size);
        ::ImGui::PopStyleColor(2);

        return result;
    }

    // Draw a colored icon button
    SE_API_RUNTIME bool FlatIconButton(char const *pIcon, char const *pLabel, SE::Color const &iconColor = SE::Color(::ImGui::GetStyle().Colors[ImGuiCol_Text]), SE::Float2 const &size = SE::Float2(0, 0), bool shouldCenterContents = false);

    // Button with extra drop down options - returns true if the primary button was pressed
    SE_API_RUNTIME bool IconButtonWithDropDown(char const *widgetID, SE::StringView icon, SE::StringView buttonLabel, SE::Color const &iconColor, float buttonWidth, void (&comboCallback)(), bool shouldCenterContents = false);

    // Toggle button
    SE_API_RUNTIME bool ToggleButton(char const *pOnLabel, char const *pOffLabel, bool &value, SE::Float2 const &size = SE::Float2(0, 0), SE::Color const &onColor = SE::GUI::Style::s_colorAccent0, SE::Color const &offColor = SE::Color(::ImGui::GetStyle().Colors[ImGuiCol_Text]));

    // Toggle button
    SE_API_RUNTIME bool FlatToggleButton(char const *pOnLabel, char const *pOffLabel, bool &value, SE::Float2 const &size = SE::Float2(0, 0), SE::Color const &onColor = SE::GUI::Style::s_colorAccent0, SE::Color const &offColor = SE::Color(::ImGui::GetStyle().Colors[ImGuiCol_Text]));

    // Draw an arrow between two points
    SE_API_RUNTIME void DrawArrow(ImDrawList *pDrawList, SE::Float2 const &arrowStart, SE::Float2 const &arrowEnd, SE::Color const &color, float arrowWidth, float arrowHeadWidth = 5.0f);

    // Draw an overlaid icon in a window, returns true if clicked
    SE_API_RUNTIME bool DrawOverlayIcon(SE::Float2 const &iconPos, char icon[4], void *iconID, bool isSelected = false, SE::Color const &selectedColor = SE::Color(::ImGui::GetStyle().Colors[ImGuiCol_ButtonActive]));

    // Draw a basic spinner
    SE_API_RUNTIME bool DrawSpinner(char const *pLabel, SE::Color const &color = SE::Color(ImGui::GetStyle().Colors[ImGuiCol_Text]), SE::Float2 size = SE::Float2(0, 0), float thickness = 3.0f, float padding = ::ImGui::GetStyle().FramePadding.y);

    //-------------------------------------------------------------------------

    SE_API_RUNTIME bool InputFloat2(char const *pID, SE::Float2 &value, float width = -1, bool readOnly = false);
    SE_API_RUNTIME bool InputFloat3(char const *pID, SE::Float3 &value, float width = -1, bool readOnly = false);
    SE_API_RUNTIME bool InputFloat4(char const *pID, SE::Float4 &value, float width = -1, bool readOnly = false);

    SE_API_RUNTIME bool InputTransform(SE::Transform &value, float width = -1, bool readOnly = false);

	// ImGui::InputText() with std::SE::String
	// Because text input needs dynamic resizing, we need to setup a callback to grow the capacity
	SE_API_RUNTIME bool  InputText(const char* label, SE::String& str, ImGuiInputTextFlags flags = 0, void* user_data = nullptr);
	SE_API_RUNTIME bool  InputTextMultiline(const char* label, SE::String& str, const SE::Float2& size = SE::Float2(0, 0), ImGuiInputTextFlags flags = 0, void* user_data = nullptr);
	SE_API_RUNTIME bool  InputTextWithHint(const char* label, const char* hint, SE::String& str, ImGuiInputTextFlags flags = 0, void* user_data = nullptr);

	//-------------------------------------------------------------------------

	SE_API_RUNTIME bool BeginCombo(const SE::StringView label, const SE::StringView preview_value, ImGuiComboFlags flags = 0);

	// "bool selected" carry the selection state (read-only). Selectable() is clicked is returns true so you can modify your selection state. size.x==0.0: use remaining width, size.x>0.0: specify width. size.y==0.0: use label height, size.y>0.0: specify height
	SE_API_RUNTIME bool Selectable(const SE::StringView label, bool selected = false, ImGuiSelectableFlags flags = 0, const SE::Float2& size = SE::Float2(0, 0));
	// "bool* p_selected" point to the selection state (read-write), as a convenient helper.
	SE_API_RUNTIME bool Selectable(const SE::StringView label, bool* p_selected, ImGuiSelectableFlags flags = 0, const SE::Float2& size = SE::Float2(0, 0));


	//-------------------------------------------------------------------------

    static void HelpMarker(const char *pHelpText)
    {
        ::ImGui::TextDisabled(ICON_HELP_CIRCLE_OUTLINE);
        if (::ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort) && ::ImGui::BeginTooltip())
        {
            ::ImGui::PushTextWrapPos(::ImGui::GetFontSize() * 35.0f);
            ::ImGui::TextUnformatted(pHelpText);
            ::ImGui::PopTextWrapPos();
            ::ImGui::EndTooltip();
        }
    }

    //-------------------------------------------------------------------------
    // Images
    //-------------------------------------------------------------------------

    SE_API_RUNTIME ImTextureID ToIm(SE::GPUTexture const &texture);

    SE_API_RUNTIME ImTextureID ToIm(SE::GPUTexture const *pTexture);

    SE_API_RUNTIME void Image(SE::Texture* img, SE::Float2 size, SE::Float2 const &uv0 = SE::Float2(0, 0),
                      SE::Float2 const &uv1 = SE::Float2(1, 1),
                      SE::Color const &tintColor = SE::Colors::White,
                      SE::Color const &borderColor = SE::Colors::Transparent);

	SE_API_RUNTIME void Image(SE::GPUTexture* img, SE::Float2 size, SE::Float2 const &uv0 = SE::Float2(0, 0),
				  SE::Float2 const &uv1 = SE::Float2(1, 1),
				  SE::Color const &tintColor = SE::Colors::White,
				  SE::Color const &borderColor = SE::Colors::Transparent);

/*    SE_API_RUNTIME void ImageButton(char const *pButtonID, ImageInfo const &img,
                            SE::Float2 const &uv0 = SE::Float2(0, 0),
                            SE::Float2 const &uv1 = SE::Float2(1, 1),
                            SE::Color const &backgroundColor = Colors::Transparent,
                            SE::Color const &tintColor = Colors::White);*/

    //-------------------------------------------------------------------------

	// A simple 3D gizmo to show the orientation of a camera in a scene
    struct SE_API_RUNTIME OrientationGuide
    {
        constexpr static float const g_windowPadding = 4.0f;
        constexpr static float const g_windowRounding = 2.0f;
        constexpr static float const g_guideDimension = 55.0f;
        constexpr static float const g_axisHeadRadius = 3.0f;
        constexpr static float const g_axisHalfLength = (g_guideDimension / 2) - g_axisHeadRadius - 4.0f;
        constexpr static float const g_worldRenderDistanceZ = 5.0f;
        constexpr static float const g_axisThickness = 2.0f;

    public:
        static SE::Float2 GetSize() { return SE::Float2(g_guideDimension, g_guideDimension); }
        static float GetWidth() { return g_guideDimension / 2; }
        static void Draw(SE::Float2 const &guideOrigin, SE::Viewport const &viewport);
    };

	//-------------------------------------------------------------------------
	// Style
	//-------------------------------------------------------------------------

	//-------------------------------------------------------------------------
	// Tree Node
	//-------------------------------------------------------------------------

	SE_API_RUNTIME bool TreeNode(SE::StringView label, ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_None);
	SE_API_RUNTIME void TreePush(SE::StringView str_id);

	SE_API_RUNTIME bool TreeNode(uint32 id, SE::StringView label, ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_None);

	//-------------------------------------------------------------------------
	// Docking and Window
	//-------------------------------------------------------------------------

	SE_API_RUNTIME void DockingBuilderWindow(SE::StringView window_name, ImGuiID node_id);

	SE_API_RUNTIME ImGuiWindow* FindWindowByName(SE::StringView window_name);

	SE_API_RUNTIME bool Begin(SE::StringView window_name, bool* p_open = NULL, ImGuiWindowFlags flags = 0);
}