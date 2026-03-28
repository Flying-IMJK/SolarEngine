#include "CurveEditor.h"
#include "Runtime/SGUI/GUILayout.h"
#include "Core/Math/Curves.h"

//-------------------------------------------------------------------------

namespace SE::GUI
{
    Color32 const CurveEditor::s_curveColor(255, 255, 255, 255);
    Color32 const CurveEditor::s_curvePointHandleColor = Colors::White;
    Color32 const CurveEditor::s_curveSelectedPointHandleColor = Colors::Yellow;
    Color32 const CurveEditor::s_curveInTangentHandleColor(0xFF90EE90);
    Color32 const CurveEditor::s_curveOutTangentHandleColor(0xFF32CD32);

    //-------------------------------------------------------------------------

    CurveEditor::CurveEditor(FloatCurve &curve)
        : m_curve(curve)
    {
        m_curve.RegeneratePointIDs();
        ViewEntireCurve();
    }

    //-------------------------------------------------------------------------

    void CurveEditor::CreatePoint(float parameter, float value)
    {
        StartEditing();
        m_curve.AddPoint(parameter, value);
        ClearSelectedPoint();
        StopEditing();
    }

    void CurveEditor::DeletePoint(int32 pointIdx)
    {
        ENGINE_ASSERT(pointIdx >= 0 && pointIdx < m_curve.GetNumPoints());
        StartEditing();
        m_curve.RemovePoint(pointIdx);
        ClearSelectedPoint();
        StopEditing();
    }

    //-------------------------------------------------------------------------

    void CurveEditor::InitializeDrawingState()
    {
        m_windowPos = ::ImGui::GetWindowPos();
        m_canvasStart = m_windowPos + Float2(::ImGui::GetCursorPos());
//        m_canvasEnd = m_windowPos + Float2(::ImGui::GetWindowContentRegionMax());
        m_canvasWidth = m_canvasEnd.x - m_canvasStart.x;
        m_canvasHeight = m_canvasEnd.y - m_canvasStart.y;

        m_curveCanvasStart = m_canvasStart;
        m_curveCanvasEnd = m_canvasEnd - Float2(s_gridLegendWidth, s_gridLegendHeight);
        m_curveCanvasWidth = m_curveCanvasEnd.x - m_curveCanvasStart.x;
        m_curveCanvasHeight = m_curveCanvasEnd.y - m_curveCanvasStart.y;

        m_horizontalRangeLength = m_horizontalViewRange.GetLength();
        m_verticalRangeLength = m_verticalViewRange.GetLength();
        m_pixelsPerUnitHorizontal = m_curveCanvasWidth / m_horizontalRangeLength;
        m_pixelsPerUnitVertical = m_curveCanvasHeight / m_verticalRangeLength;
    }

    //-------------------------------------------------------------------------

    void CurveEditor::ViewEntireCurve()
    {
        ViewEntireHorizontalRange();
        ViewEntireVerticalRange();
    }

    void CurveEditor::ViewEntireHorizontalRange()
    {
        if (m_curve.GetNumPoints() == 0)
        {
            m_horizontalViewRange = FloatRange(-s_fitViewExtraMarginPercentage, 1.0f + s_fitViewExtraMarginPercentage);
        }
        else
        {
            m_horizontalViewRange = m_curve.GetParameterRange();
            float const parameterRangeLength = m_horizontalViewRange.GetLength();
            if (Math::IsNearZero(parameterRangeLength))
            {
                m_horizontalViewRange.begin -= 0.5f;
                m_horizontalViewRange.end += 0.5f;
            }
            else
            {
                m_horizontalViewRange.begin -= parameterRangeLength * s_fitViewExtraMarginPercentage;
                m_horizontalViewRange.end += parameterRangeLength * s_fitViewExtraMarginPercentage;
            }
        }

        ENGINE_ASSERT(!Math::IsNearZero(m_horizontalViewRange.GetLength()));
    }

    void CurveEditor::ViewEntireVerticalRange()
    {
        if (m_curve.GetNumPoints() == 0)
        {
            m_verticalViewRange = FloatRange(-s_fitViewExtraMarginPercentage, 1.0f + s_fitViewExtraMarginPercentage);
        }
        else
        {
            m_verticalViewRange = m_curve.GetValueRange();

            float const valueRangeLength = m_verticalViewRange.GetLength();
            if (Math::IsNearZero(valueRangeLength))
            {
                m_verticalViewRange.begin -= 0.5f;
                m_verticalViewRange.end += 0.5f;
            }
            else
            {
                m_verticalViewRange.begin -= valueRangeLength * s_fitViewExtraMarginPercentage;
                m_verticalViewRange.end += valueRangeLength * s_fitViewExtraMarginPercentage;
            }
        }

        ENGINE_ASSERT(!Math::IsNearZero(m_verticalViewRange.GetLength()));
    }

    //-------------------------------------------------------------------------

    void CurveEditor::DrawToolbar()
    {
        // View Controls
        //-------------------------------------------------------------------------

        if (ImGui::Button(SE_TEXT(ICON_FIT_TO_PAGE_OUTLINE), SE_TEXT("Fit entire curve")))
        {
            ViewEntireCurve();
        }

        ::ImGui::SameLine();

        if (ImGui::Button(SE_TEXT(ICON_ARROW_EXPAND_HORIZONTAL), SE_TEXT("Fit curve horizontally")))
        {
            ViewEntireHorizontalRange();
        }

        ::ImGui::SameLine();

        if (ImGui::Button(SE_TEXT(ICON_ARROW_EXPAND_VERTICAL), SE_TEXT("Fit curve vertically")))
        {
            ViewEntireVerticalRange();
        }

        // Point Editor
        //-------------------------------------------------------------------------

        ::ImGui::SameLine(::ImGui::GetContentRegionAvail().x - 194, 0);
        ImGui::Label(SE_TEXT("Point:"));
        ::ImGui::SameLine();

        ::ImGui::BeginDisabled(m_selectedPointIdx == -1);
        ::ImGui::SetNextItemWidth(150);
        ::ImGui::InputFloat2("##PointEditor", &m_selectedPointValue.x, "%.2f");
        if (::ImGui::IsItemDeactivatedAfterEdit())
        {
            StartEditing();
            m_curve.EditPoint(m_selectedPointIdx, m_selectedPointValue.x, m_selectedPointValue.y);
            StopEditing();
            ViewEntireCurve();
        }
        ::ImGui::EndDisabled();
    }

    void CurveEditor::DrawGridAndLegend(ImDrawList *pDrawList)
    {
        char legendString[10];
        pDrawList->AddRectFilled(m_canvasStart, m_canvasEnd, Color32(Style::s_colorGray7));

        int32 const numVerticalLines = Math::FloorToInt(m_curveCanvasWidth / s_pixelsPerGridBlock);
        for (auto i = 0; i <= numVerticalLines; i++)
        {
            Float2 lineStart(m_canvasStart.x + (i * s_pixelsPerGridBlock), m_canvasStart.y);
            Float2 lineEnd(lineStart.x, m_canvasEnd.y - s_gridLegendHeight);
            pDrawList->AddLine(lineStart, lineEnd, Color32(Style::s_colorGray5));


            float const legendValue = m_horizontalViewRange.GetPercentageThrough((lineStart.x - m_canvasStart.x) / m_curveCanvasWidth);
            fmt::format_to_n(legendString, 10, "{0:.2f}", legendValue);

            {
                ScopedFont sf(Font::Small);
                Float2 const textSize = ::ImGui::CalcTextSize(legendString);
                pDrawList->AddText(ImVec2(lineStart.x - (textSize.x / 2), lineEnd.y), 0xFFFFFFFF, legendString);
            }
        }

        int32 const numHorizontalLines = Math::FloorToInt(m_curveCanvasHeight / s_pixelsPerGridBlock);
        for (auto i = 0; i <= numHorizontalLines; i++)
        {
            Float2 const lineStart(m_canvasStart.x, m_canvasStart.y + (i * s_pixelsPerGridBlock));
            Float2 const lineEnd(m_canvasEnd.x - s_gridLegendWidth, lineStart.y);
            pDrawList->AddLine(lineStart, lineEnd, Color32(Style::s_colorGray5));

            float const legendValue = m_verticalViewRange.GetPercentageThrough(1.0f - ((lineEnd.y - m_canvasStart.y) / m_curveCanvasHeight));
			fmt::format_to_n(legendString, 10, "{0:.2f}", legendValue);

            {
                ScopedFont sf(Font::Small);
                Float2 const textSize = ::ImGui::CalcTextSize(legendString);
                pDrawList->AddText(ImVec2(lineEnd.x, lineEnd.y - (textSize.y / 2)), 0xFFFFFFFF, legendString);
            }
        }
    }

    void CurveEditor::DrawCurve(ImDrawList *pDrawList)
    {
        int32 const numPointsToDraw = Math::RoundToInt(m_curveCanvasWidth / 2) + 1;
        float const stepT = m_horizontalRangeLength / (numPointsToDraw - 1);

        List<ImVec2> curvePoints;
        for (auto i = 0; i < numPointsToDraw; i++)
        {
            float const t = m_horizontalViewRange.begin + (i * stepT);
            Float2 curvePoint(t, m_curve.Evaluate(t));
            curvePoint.x = m_curveCanvasStart.x + (m_horizontalViewRange.GetPercentageThrough(curvePoint.x) * m_curveCanvasWidth);
            curvePoint.y = m_curveCanvasEnd.y - (m_verticalViewRange.GetPercentageThrough(curvePoint.y) * m_curveCanvasHeight);
            curvePoints.Add(curvePoint);
        }

        pDrawList->AddPolyline(curvePoints.Get(), numPointsToDraw, s_curveColor, 0, 2.0f);
    }

    bool CurveEditor::DrawInTangentHandle(ImDrawList *pDrawList, int32 pointIdx)
    {
        ENGINE_ASSERT(pointIdx >= 0 && pointIdx < m_curve.GetNumPoints());
        FloatCurve::Point const &point = m_curve.GetPoint(pointIdx);
        Float2 const pointCenter = GetScreenPosFromCurvePos(point);

        //-------------------------------------------------------------------------

        // Calculate the tangent offset based on the current slope (invert it since it's incoming)
        Float2 tangentOffset(-m_pixelsPerUnitHorizontal, -point.m_inTangent * m_pixelsPerUnitVertical);
        tangentOffset.Normalize();
        tangentOffset *= s_slopeHandleLength;

        Float2 tangentHandleCenter;
        tangentHandleCenter.x = (pointCenter.x + tangentOffset.x);
        tangentHandleCenter.y = (pointCenter.y - tangentOffset.y);

        // Draw visual handle
        pDrawList->AddLine(pointCenter, tangentHandleCenter, s_curveInTangentHandleColor);
        pDrawList->AddCircleFilled(tangentHandleCenter, s_handleRadius, s_curveInTangentHandleColor);

        // Draw handle button
        ::ImGui::SetCursorScreenPos(tangentHandleCenter - Float2(s_handleRadius, s_handleRadius));
        ::ImGui::InvisibleButton(StringAnsi::Format("in_{0}", point.m_ID).Get(), ImVec2(2 * s_handleRadius, 2 * s_handleRadius));

        //-------------------------------------------------------------------------

        bool isCurrentlyEditing = false;
        if (::ImGui::IsItemActive() && ::ImGui::IsMouseDragging(0))
        {
            if (!m_isEditing)
            {
                StartEditing();
            }

            // Get visual tangent offset
            auto const &io = ::ImGui::GetIO();
            Float2 tangentCanvasOffset(io.MousePos.x - pointCenter.x, io.MousePos.y - pointCenter.y);
            tangentCanvasOffset.x = Math::Min(tangentCanvasOffset.x, 0.0f); // Lock outgoing tangents to the left hemisphere
            tangentCanvasOffset.y = -tangentCanvasOffset.y;                 // Handle y flip here

            // Convert to curve units (invert direction here)
            tangentCanvasOffset.x = -tangentCanvasOffset.x / m_pixelsPerUnitHorizontal;
            tangentCanvasOffset.y = -tangentCanvasOffset.y / m_pixelsPerUnitVertical;
            tangentCanvasOffset = Float2::Normalize(tangentCanvasOffset);

            // Calculate and clamp tangent
            float newTangentSlope;
            if (Math::IsNearZero(tangentCanvasOffset.x))
            {
                newTangentSlope = (tangentCanvasOffset.y < 0) ? -30.0f : 30.0f;
            }
            else
            {
                newTangentSlope = tangentCanvasOffset.y / tangentCanvasOffset.x;
            }

            m_curve.SetPointInTangent(pointIdx, newTangentSlope);
            isCurrentlyEditing = true;
        }
        return isCurrentlyEditing;
    }

    bool CurveEditor::DrawOutTangentHandle(ImDrawList *pDrawList, int32 pointIdx)
    {
        ENGINE_ASSERT(pointIdx >= 0 && pointIdx < m_curve.GetNumPoints());
        FloatCurve::Point const &point = m_curve.GetPoint(pointIdx);
        Float2 const pointCenter = GetScreenPosFromCurvePos(point);

        //-------------------------------------------------------------------------

        // Calculate the tangent offset based on the current slope
        Float2 tangentOffset(m_pixelsPerUnitHorizontal, point.m_outTangent * m_pixelsPerUnitVertical);
        tangentOffset.Normalize();
        tangentOffset *= s_slopeHandleLength;

        Float2 tangentHandleCenter;
        tangentHandleCenter.x = (pointCenter.x + tangentOffset.x);
        tangentHandleCenter.y = (pointCenter.y - tangentOffset.y);

        // Draw visual handle
        pDrawList->AddLine(pointCenter, tangentHandleCenter, s_curveOutTangentHandleColor);
        pDrawList->AddCircleFilled(tangentHandleCenter, s_handleRadius, s_curveOutTangentHandleColor);

        // Draw handle button
        ::ImGui::SetCursorScreenPos(tangentHandleCenter - Float2(s_handleRadius, s_handleRadius));
        ::ImGui::InvisibleButton(StringAnsi::Format("out_{0}", point.m_ID).Get(), ImVec2(2 * s_handleRadius, 2 * s_handleRadius));

        //-------------------------------------------------------------------------

        bool isCurrentlyEditing = false;
        if (::ImGui::IsItemActive() && ::ImGui::IsMouseDragging(0))
        {
            if (!m_isEditing)
            {
                StartEditing();
            }

            // Get visual tangent offset
            auto const &io = ::ImGui::GetIO();
            Float2 tangentCanvasOffset(io.MousePos.x - pointCenter.x, io.MousePos.y - pointCenter.y);
            tangentCanvasOffset.x = Math::Max(0.0f, tangentCanvasOffset.x); // Lock outgoing tangents to the right hemisphere
            tangentCanvasOffset.y = -tangentCanvasOffset.y;                 // Handle y flip here

            // Convert to curve units
            tangentCanvasOffset.x = tangentCanvasOffset.x / m_pixelsPerUnitHorizontal;
            tangentCanvasOffset.y = tangentCanvasOffset.y / m_pixelsPerUnitVertical;
            tangentCanvasOffset = Float2::Normalize(tangentCanvasOffset);

            // Calculate and clamp tangent
            float newTangentSlope;
            if (Math::IsNearZero(tangentCanvasOffset.x))
            {
                newTangentSlope = (tangentCanvasOffset.y < 0) ? -30.0f : 30.0f;
            }
            else
            {
                newTangentSlope = tangentCanvasOffset.y / tangentCanvasOffset.x;
            }

            m_curve.SetPointOutTangent(pointIdx, newTangentSlope);

            if (point.m_tangentMode == FloatCurve::Locked)
            {
                m_curve.SetPointInTangent(pointIdx, newTangentSlope);
            }

            isCurrentlyEditing = true;
        }
        return isCurrentlyEditing;
    }

    bool CurveEditor::DrawPointHandle(ImDrawList *pDrawList, int32 pointIdx)
    {
        FloatCurve::Point const &point = m_curve.GetPoint(pointIdx);
        Float2 const pointCenter = GetScreenPosFromCurvePos(point);

        pDrawList->AddCircleFilled(pointCenter, s_handleRadius, (pointIdx == m_selectedPointIdx) ? s_curveSelectedPointHandleColor : s_curvePointHandleColor);

        //-------------------------------------------------------------------------
        ::ImGui::SetCursorScreenPos(pointCenter - Float2(s_handleRadius, s_handleRadius));
        if (::ImGui::InvisibleButton(StringAnsi::Format("point_{0}", point.m_ID).Get(), Float2(2 * s_handleRadius, 2 * s_handleRadius)))
        {
            SelectPoint(pointIdx);
        }

        if (::ImGui::IsItemActive() || ::ImGui::IsItemHovered())
        {
            ::ImGui::SetTooltip("%4.3f, %4.3f", point.m_parameter, point.m_value);
        }

        if (::ImGui::IsItemHovered() && ::ImGui::IsMouseClicked(ImGuiMouseButton_Right))
        {
            SelectPoint(pointIdx);
            ::ImGui::OpenPopup(s_pointContextMenuName);
        }

        //-------------------------------------------------------------------------

        bool isCurrentlyEditing = false;
        if (::ImGui::IsItemActive() && ::ImGui::IsMouseDragging(0))
        {
            if (!m_isEditing)
            {
                StartEditing();
            }

            SelectPoint(pointIdx);

            ImRect const curveRect = GetCurveRect();
            ImVec2 const clampedMousePos = ImGui::ClampToRect(curveRect, ::ImGui::GetMousePos());
            float const updatedParameter = ((clampedMousePos.x - curveRect.Min.x) / m_pixelsPerUnitHorizontal) + m_horizontalViewRange.begin;
            float const updatedValue = m_verticalViewRange.GetLength() - ((clampedMousePos.y - curveRect.Min.y) / m_pixelsPerUnitVertical) + m_verticalViewRange.begin;

            m_curve.EditPoint(pointIdx, updatedParameter, updatedValue);
            isCurrentlyEditing = true;
        }
        return isCurrentlyEditing;
    }

    //-------------------------------------------------------------------------

    void CurveEditor::StartEditing()
    {
        ENGINE_ASSERT(!m_isEditing);
        m_isEditing = true;
        m_valueBeforeEdit = m_curve.ToString();
    }

    void CurveEditor::StopEditing()
    {
        ENGINE_ASSERT(m_isEditing);
        m_isEditing = false;
        m_wasCurveEdited = true;
    }

    void CurveEditor::SelectPoint(int32 pointIdx)
    {
        ENGINE_ASSERT(pointIdx >= 0 && pointIdx < m_curve.GetNumPoints());
        m_selectedPointIdx = pointIdx;
        m_selectedPointValue = Float2(m_curve.GetPoint(pointIdx).m_parameter, m_curve.GetPoint(pointIdx).m_value);
        m_wasPointSelected = true;
    }

    void CurveEditor::ClearSelectedPoint()
    {
        m_selectedPointIdx = -1;
        m_selectedPointValue = Float2::Zero;
    }

    //-------------------------------------------------------------------------

    void CurveEditor::DrawContextMenus(bool isMiniView)
    {
        if (::ImGui::BeginPopup(s_gridContextMenuName))
        {
            ::ImGui::SeparatorText("Editing");

            if (::ImGui::MenuItem(ICON_PLUS " Create Point"))
            {
                Float2 const mouseCurvePos = GetCurvePosFromScreenPos(::ImGui::GetWindowPos());
                CreatePoint(mouseCurvePos.x, mouseCurvePos.y);
            }

            //-------------------------------------------------------------------------

            ::ImGui::SeparatorText("Zoom");

            if (::ImGui::MenuItem(ICON_FIT_TO_PAGE_OUTLINE " Fit entire curve"))
            {
                ViewEntireCurve();
            }

            if (::ImGui::MenuItem(ICON_ARROW_EXPAND_HORIZONTAL " Fit curve horizontally"))
            {
                ViewEntireHorizontalRange();
            }

            if (::ImGui::MenuItem(ICON_ARROW_EXPAND_VERTICAL " Fit curve vertically"))
            {
                ViewEntireVerticalRange();
            }

            //-------------------------------------------------------------------------

            if (isMiniView)
            {
                ::ImGui::SeparatorText("Editing");

                if (::ImGui::MenuItem(ICON_PLAYLIST_EDIT " Open Full Curve Editor"))
                {
                    m_requestOpenFullEditor = true;
                }
            }

            //-------------------------------------------------------------------------

            ::ImGui::EndPopup();
        }

        //-------------------------------------------------------------------------

        if (::ImGui::BeginPopup(s_pointContextMenuName))
        {
            ENGINE_ASSERT(m_selectedPointIdx != -1);

            auto const &point = m_curve.GetPoint(m_selectedPointIdx);

            ::ImGui::SetNextItemWidth(-1);
            ::ImGui::InputFloat2("##PointEditor", &m_selectedPointValue.x, "%.2f");
            if (::ImGui::IsItemDeactivatedAfterEdit())
            {
                StartEditing();
                m_curve.EditPoint(m_selectedPointIdx, m_selectedPointValue.x, m_selectedPointValue.y);
                StopEditing();
                ViewEntireCurve();
            }

            if (point.m_tangentMode == FloatCurve::Free)
            {
                if (::ImGui::MenuItem(ICON_LOCK " Lock Tangents"))
                {
                    StartEditing();
                    m_curve.SetPointTangentMode(m_selectedPointIdx, FloatCurve::Locked);
                    StopEditing();
                }
            }

            if (point.m_tangentMode == FloatCurve::Locked)
            {
                if (::ImGui::MenuItem(ICON_LOCK_OPEN " Unlock Tangents"))
                {
                    StartEditing();
                    m_curve.SetPointTangentMode(m_selectedPointIdx, FloatCurve::Free);
                    StopEditing();
                }
            }

            if (::ImGui::MenuItem(ICON_DELETE " Delete Point"))
            {
                DeletePoint(m_selectedPointIdx);
            }
            ::ImGui::EndPopup();
        }
    }

    void CurveEditor::HandleFrameInput(bool isMiniView)
    {
        auto const &io = ::ImGui::GetIO();
        if (::ImGui::IsWindowHovered())
        {
            // Position tooltip
            //-------------------------------------------------------------------------

            if (io.KeyShift)
            {
                Float2 const mouseCurvePos = GetCurvePosFromScreenPos(io.MousePos);
                ::ImGui::SetTooltip("%4.3f, %4.3f", mouseCurvePos.x, mouseCurvePos.y);
            }

            // Pan view
            //-------------------------------------------------------------------------

            if (::ImGui::IsMouseDragging(ImGuiMouseButton_Middle))
            {
                m_horizontalViewRange.ShiftRange(-io.MouseDelta.x / m_pixelsPerUnitHorizontal);
                m_verticalViewRange.ShiftRange(io.MouseDelta.y / m_pixelsPerUnitVertical);
            }

            // Zoom View
            //-------------------------------------------------------------------------

            if (io.MouseWheel != 0)
            {
                float const scale = io.MouseWheel * s_zoomScaleStep;
                bool const zoomHorizontal = (!io.KeyAlt && io.KeyCtrl) || (!io.KeyAlt && !io.KeyCtrl);
                bool const zoomVertical = (io.KeyAlt && !io.KeyCtrl) || (!io.KeyAlt && !io.KeyCtrl);

                // Horizontal zoom
                if (zoomHorizontal)
                {
                    float const mp = m_horizontalViewRange.GetMidpoint();
                    float const rl = m_horizontalViewRange.GetLength();
                    float const nhl = (rl * (1 - scale)) / 2;
                    m_horizontalViewRange.begin = mp - nhl;
                    m_horizontalViewRange.end = mp + nhl;
                }

                // Vertical zoom
                if (zoomVertical)
                {
                    float const mp = m_verticalViewRange.GetMidpoint();
                    float const rl = m_verticalViewRange.GetLength();
                    float const nhl = (rl * (1 - scale)) / 2;
                    m_verticalViewRange.begin = mp - nhl;
                    m_verticalViewRange.end = mp + nhl;
                }
            }

            // Mouse Actions
            //-------------------------------------------------------------------------

            // Clear point selection
            if (::ImGui::IsMouseClicked(ImGuiMouseButton_Left) || ::ImGui::IsMouseClicked(ImGuiMouseButton_Right))
            {
                if (!::ImGui::IsPopupOpen(s_pointContextMenuName) && !m_wasPointSelected)
                {
                    ClearSelectedPoint();
                }
            }

            if (isMiniView)
            {
                if (::ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
                {
                    m_requestOpenFullEditor = true;
                }
            }

            // Key actions
            //-------------------------------------------------------------------------

            if (m_selectedPointIdx != -1 && ::ImGui::IsKeyPressed(ImGuiKey_Delete))
            {
                DeletePoint(m_selectedPointIdx);
            }

            if (::ImGui::IsKeyPressed(ImGuiKey_Space))
            {
                Float2 const mouseCurvePos = GetCurvePosFromScreenPos(io.MousePos);
                CreatePoint(mouseCurvePos.x, mouseCurvePos.y);
            }

            // Context Menu
            //-------------------------------------------------------------------------

            if (!::ImGui::IsPopupOpen(s_pointContextMenuName) && ::ImGui::IsMouseClicked(ImGuiMouseButton_Right))
            {
                ::ImGui::OpenPopup(s_gridContextMenuName);
            }
        }
    }

    bool CurveEditor::UpdateAndDraw(bool isMiniView)
    {
        ENGINE_ASSERT(!Math::IsNearZero(m_horizontalViewRange.GetLength()) && !Math::IsNearZero(m_verticalViewRange.GetLength()));

        //-------------------------------------------------------------------------

        // Ensure selected point index is always valid since the curve could be externally edited too
        if (m_selectedPointIdx >= m_curve.GetNumPoints())
        {
            ClearSelectedPoint();
        }

        // Clear the PreEditState if we're not editing and it is set
        if (!m_isEditing && m_valueBeforeEdit.IsEmpty())
        {
            m_valueBeforeEdit.Clear();
        }

        // Clear selection/edit frame state
        m_wasPointSelected = false;
        m_wasCurveEdited = false;

        // Toolbar has to be drawn first before resetting the frame state since we need to shift the cursor position
        if (!isMiniView)
        {
            DrawToolbar();
        }
        InitializeDrawingState();

        //-------------------------------------------------------------------------

        ::ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
        if (::ImGui::BeginChild("CurveView", m_canvasEnd - m_canvasStart, false, ImGuiWindowFlags_NoScrollbar))
        {
            auto pDrawList = ::ImGui::GetWindowDrawList();

            // Draw Grid and legend
            //-------------------------------------------------------------------------

            DrawGridAndLegend(pDrawList);

            // Draw Curve and Points
            //-------------------------------------------------------------------------

            auto const curveRect = GetCurveRect();
            pDrawList->PushClipRect(curveRect.Min, curveRect.Max);

            DrawCurve(pDrawList);

            bool stillEditing = false;
            int32 const numPoints = m_curve.GetNumPoints();
            for (int32 i = 0; i < numPoints; i++)
            {
                if (i != 0 && m_curve.GetPoint(i).m_tangentMode == FloatCurve::Free)
                {
                    stillEditing |= DrawInTangentHandle(pDrawList, i);
                }

                if (i != numPoints - 1)
                {
                    stillEditing |= DrawOutTangentHandle(pDrawList, i);
                }

                stillEditing |= DrawPointHandle(pDrawList, i);
            }

            // If we completed an edit operation
            if (m_isEditing && !stillEditing)
            {
                StopEditing();
            }

            pDrawList->PopClipRect();

            //-------------------------------------------------------------------------

            HandleFrameInput(isMiniView);
            DrawContextMenus(isMiniView);
        }
        ::ImGui::EndChild();
        ::ImGui::PopStyleColor();

        //-------------------------------------------------------------------------

        // Draw full editor in a modal window
        if (isMiniView)
        {
            if (m_requestOpenFullEditor)
            {
                ::ImGui::OpenPopup("Curve Editor");
                m_requestOpenFullEditor = false;
            }

            bool isOpen = true;
            ::ImGui::SetNextWindowSize(ImVec2(800, 600), ImGuiCond_FirstUseEver);
            if (::ImGui::BeginPopupModal("Curve Editor", &isOpen))
            {
                UpdateAndDraw(false);
                ::ImGui::EndPopup();
            }
        }

        //-------------------------------------------------------------------------

        return m_wasCurveEdited;
    }
}