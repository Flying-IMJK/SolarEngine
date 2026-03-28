#pragma once

#include "Runtime/API.h"
#include "Runtime/SGUI/GUILayout.h"
#include "Core/Math/FloatCurve.h"
#include "Core/Utilities/Timers.h"
#include "Core/Types/UID.h"

//-------------------------------------------------------------------------
// Curve Editor
//-------------------------------------------------------------------------
// Controls:
//  hold middle mouse - pan view
//  mouse wheel - zoom view
//  ctrl + mouse wheel - zoom horizontal view
//  alt + mouse wheel - zoom vertical views
//  hold shift - show cursor position tooltip

namespace SE::GUI
{
    struct CurveEditorContext;

    //-------------------------------------------------------------------------

    class SE_API_RUNTIME CurveEditor
    {
        constexpr static float const s_fitViewExtraMarginPercentage = 0.1f;
        constexpr static float const s_handleRadius = 6.0f;
        constexpr static float const s_zoomScaleStep = 0.1f;
        constexpr static float const s_pixelsPerGridBlock = 45;
        constexpr static float const s_slopeHandleLength = 45.0f;
        constexpr static float const s_gridLegendWidth = 30;
        constexpr static float const s_gridLegendHeight = 16;

        constexpr static char const* const s_pointContextMenuName = "PointCtxMenu";
        constexpr static char const* const s_gridContextMenuName = "GridCtxMenu";

        static Color32 const s_curveColor;
        static Color32 const s_curvePointHandleColor;
        static Color32 const s_curveSelectedPointHandleColor;
        static Color32 const s_curveInTangentHandleColor;
        static Color32 const s_curveOutTangentHandleColor;

    public:

        CurveEditor( FloatCurve& curve );

        // Call this to notify the curve editor that the curve has been externally updated
        void OnCurveExternallyUpdated() { m_curve.RegeneratePointIDs(); }

        // Resets the view and clears the selection 
        void ResetView() { ViewEntireCurve(); m_selectedPointIdx = -1; }

        // Draw the the curve editor and edit the curve - returns true if a curve edit completes - you can retrieve the previous state of the curve using the "GetPreEditCurveState()" function
        // Pass true to this function to add a button that will open the same editor in a modal dialog (useful for property grids, etc.)
        bool UpdateAndDraw( bool isMiniView );

        // Returns the state of the curve before the edit operation, only valid on the same frame that UpdateAndDraw() returns true
        String const& GetPreEditCurveState() const {ENGINE_ASSERT( !m_isEditing && !m_valueBeforeEdit.IsEmpty() ); return m_valueBeforeEdit; }

        // Adjust view range so that the entire curve is visible
        void ViewEntireCurve();

        // Adjust view range so that the entire curve parameter range is visible
        void ViewEntireHorizontalRange();

        // Adjust view range so that the entire curve value range is visible
        void ViewEntireVerticalRange();

    private:

        void CreatePoint( float parameter, float value );
        void DeletePoint( int32 pointIdx );

        //-------------------------------------------------------------------------

        void InitializeDrawingState();

        void DrawToolbar();
        void DrawGridAndLegend( ImDrawList* pDrawList );
        void DrawCurve( ImDrawList* pDrawList );
        bool DrawInTangentHandle( ImDrawList* pDrawList, int32 pointIdx );
        bool DrawOutTangentHandle( ImDrawList* pDrawList, int32 pointIdx );
        bool DrawPointHandle( ImDrawList* pDrawList, int32 pointIdx );
        void DrawContextMenus( bool isMiniView );
        void HandleFrameInput( bool isMiniView );

        inline Float2 GetCurvePosFromScreenPos( Float2 const& screenPos ) const
        {
            ImVec2 curvePos( screenPos - m_curveCanvasStart );
            curvePos.x /= m_curveCanvasWidth;
            curvePos.y /= m_curveCanvasHeight;
            curvePos.x = m_horizontalViewRange.GetValueForPercentageThrough( curvePos.x );
            curvePos.y = m_verticalViewRange.GetValueForPercentageThrough( 1.0f - curvePos.y );
            return curvePos;
        }

        inline Float2 GetScreenPosFromCurvePos( float parameter, float value ) const
        {
            Float2 screenPos;
            screenPos.x = m_curveCanvasStart.x + ( m_horizontalViewRange.GetPercentageThrough( parameter ) * m_curveCanvasWidth );
            screenPos.y = m_curveCanvasEnd.y - ( m_verticalViewRange.GetPercentageThrough( value ) * m_curveCanvasHeight );
            return screenPos;
        }

        inline ImRect GetCurveRect() const
        {
            return ImRect( m_canvasStart, m_canvasEnd - Float2( s_gridLegendWidth, s_gridLegendHeight ) );
        }

        inline Float2 GetScreenPosFromCurvePos( Float2 const& curvePos ) const { return GetScreenPosFromCurvePos( curvePos.x, curvePos.y ); }
        inline Float2 GetScreenPosFromCurvePos( FloatCurve::Point const& point ) const { return GetScreenPosFromCurvePos( point.m_parameter, point.m_value ); }

        void StartEditing();
        void StopEditing();

        void SelectPoint( int32 pointIdx );
        void ClearSelectedPoint();

    private:

        // Persistent State
        FloatCurve&                                         m_curve;
        FloatRange                                          m_horizontalViewRange = FloatRange( 0, 1 );
        FloatRange                                          m_verticalViewRange = FloatRange( 0, 1 );
        int32                                               m_selectedPointIdx = -1;
        Float2                                              m_selectedPointValue;
        String                                              m_valueBeforeEdit;
        bool                                                m_isEditing = false;

        Float2                                              m_windowPos;
        Float2                                              m_canvasStart;
        Float2                                              m_canvasEnd;
        float                                               m_canvasWidth;
        float                                               m_canvasHeight;

        Float2                                              m_curveCanvasStart;
        Float2                                              m_curveCanvasEnd;
        float                                               m_curveCanvasWidth;
        float                                               m_curveCanvasHeight;

        float                                               m_horizontalRangeLength;
        float                                               m_verticalRangeLength;
        float                                               m_pixelsPerUnitHorizontal;
        float                                               m_pixelsPerUnitVertical;
        bool                                                m_wasPointSelected = false;
        bool                                                m_wasCurveEdited = false;
        bool                                                m_requestOpenFullEditor = false;
    };
}