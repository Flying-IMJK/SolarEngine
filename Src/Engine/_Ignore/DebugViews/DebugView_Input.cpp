#include "DebugView_Input.h"
#include "Runtime/SGUI/GUILayout.h"
#include "Runtime/UpdateContext.h"
// #include "Engine/Entity/EntityWorldUpdateContext.h"
#include "Core/Input/InputSystem.h"

//-------------------------------------------------------------------------

#if ENGINE_DEVELOPMENT_TOOLS
namespace SGE::Input
{
    namespace
    {
        static uint32 const g_controlOutlineColor = 0xFF666666;
        static uint32 const g_controlFillColor = 0xFF888888;

        static float const g_buttonWidth = 20;
        static float const g_buttonBorderThickness = 2.0f;
        static float const g_buttonBorderRounding = 4.0f;

        static float const g_analogStickRangeRadius = 50;
        static float const g_analogStickPositionRadius = 2;

        static Float2 const g_buttonBorderOffset( g_buttonBorderThickness );
        static Float2 const g_buttonDimensions( g_buttonWidth, g_buttonWidth );

        static StringID const g_controllerWindowTypeID( "ControllerWindow" );
    }

    static void DrawButton( ImDrawList* pDrawList, Float2 const& position, Float2 const& dimensions, char const* const pLabel, bool IsHeldDown, uint32 buttonColor = g_controlOutlineColor, uint32 pressedColor = g_controlFillColor )
    {
        ENGINE_ASSERT( pDrawList != nullptr );
        Float2 const buttonTopLeft = position;
        Float2 const buttonBottomRight = buttonTopLeft + dimensions;
        pDrawList->AddRect( buttonTopLeft, buttonBottomRight, buttonColor, g_buttonBorderRounding, ImDrawFlags_RoundCornersAll, g_buttonBorderThickness );

        if ( IsHeldDown )
        {
            pDrawList->AddRectFilled( buttonTopLeft + g_buttonBorderOffset, buttonTopLeft + dimensions - g_buttonBorderOffset, pressedColor, g_buttonBorderRounding - 1, ImDrawFlags_RoundCornersAll );
        }

        if ( pLabel != nullptr )
        {
            Float2 const textDimensions = ImGui::CalcTextSize( pLabel );
            Float2 const textPos = Float2( buttonTopLeft.x + ( dimensions.x / 2 ) - ( textDimensions.x / 2 ) + 1, buttonTopLeft.y + ( dimensions.y / 2 ) - ( textDimensions.y / 2 ) );
            pDrawList->AddText( textPos, 0xFFFFFFFF, pLabel );
        }
    }

    static void DrawTriggerButton( ImDrawList* pDrawList, Float2 const& position, Float2 const& dimensions, char const* const pLabel, ControllerInputState const& controllerState, bool isLeftTrigger )
    {
        ENGINE_ASSERT( pDrawList != nullptr );

        // Draw the label if set
        Float2 drawPosition = position;
        if ( pLabel != nullptr )
        {
            Float2 const textDimensions = ImGui::CalcTextSize( pLabel );
            Float2 const textPos = Float2( drawPosition.x + ( dimensions.x / 2 ) - ( textDimensions.x / 2 ) + 1, drawPosition.y + g_buttonBorderThickness );
            pDrawList->AddText( textPos, 0xFFFFFFFF, pLabel );
            drawPosition.y += textDimensions.y + 4;
        }

        // Draw the border
        Float2 const borderDimensions( dimensions.x, dimensions.y - ( drawPosition.y - position.y + 4 ) );
        Float2 const triggerTopLeft = drawPosition;
        Float2 const triggerBottomRight = triggerTopLeft + borderDimensions;
        pDrawList->AddRect( triggerTopLeft, triggerBottomRight, g_controlOutlineColor, 0.0f, ImDrawFlags_RoundCornersAll, g_buttonBorderThickness );

        // Draw the trigger values
        float const triggerValueRaw = isLeftTrigger ? controllerState.GetLeftTriggerRawValue() : controllerState.GetRightTriggerRawValue();
        if ( triggerValueRaw > 0 )
        {
            float triggerValue0;
            uint32 triggerValue0Color;

            float triggerValue1;
            uint32 triggerValue1Color;

            if ( isLeftTrigger )
            {
                triggerValue0 = controllerState.GetLeftTriggerRawValue();
                triggerValue1 = controllerState.GetLeftTriggerValue();
                triggerValue0Color = 0xFF0000FF;
                triggerValue1Color = 0xFF00FF00;
            }
            else
            {
                triggerValue0 = controllerState.GetRightTriggerValue();
                triggerValue1 = controllerState.GetRightTriggerRawValue();
                triggerValue0Color = 0xFF00FF00;
                triggerValue1Color = 0xFF0000FF;
            }

            float const valueMaxLength = borderDimensions.y - ( g_buttonBorderThickness * 2 );
            float const triggerValueWidth = ( borderDimensions.x - g_buttonBorderThickness * 2 ) / 2;
            float const triggerValue0TopLeftX = drawPosition.x + g_buttonBorderThickness;
            float const triggerValue1TopLeftX = triggerValue0TopLeftX + triggerValueWidth;
            float const triggerValue0TopLeftY = drawPosition.y + g_buttonBorderThickness + ( 1.0f - triggerValue0 ) * valueMaxLength;
            float const triggerValue1TopLeftY = drawPosition.y + g_buttonBorderThickness + ( 1.0f - triggerValue1 ) * valueMaxLength;

            Float2 const triggerValue0TopLeft( triggerValue0TopLeftX, triggerValue0TopLeftY );
            Float2 const triggerValue0BottomRight( triggerValue1TopLeftX, triggerBottomRight.y - g_buttonBorderThickness );
            pDrawList->AddRectFilled( triggerValue0TopLeft, triggerValue0BottomRight, triggerValue0Color );

            Float2 const triggerValue1TopLeft( triggerValue0TopLeftX + triggerValueWidth, triggerValue1TopLeftY );
            Float2 const triggerValue1BottomRight( triggerValue1TopLeftX + triggerValueWidth, triggerBottomRight.y - g_buttonBorderThickness );
            pDrawList->AddRectFilled( triggerValue1TopLeft, triggerValue1BottomRight, triggerValue1Color );
        }
    }

    static void DrawAnalogStick( ImDrawList* pDrawList, Float2 const position, ControllerInputDevice const& controller, bool isLeftStick )
    {
        ENGINE_ASSERT( pDrawList != nullptr );

        auto const& settings = controller.GetSettings();
        auto const& controllerState = controller.GetControllerState();

        Float2 rawValue = isLeftStick ? controllerState.GetLeftAnalogStickRawValue() : controllerState.GetRightAnalogStickRawValue();
        Float2 filteredValue = isLeftStick ? controllerState.GetLeftAnalogStickValue() : controllerState.GetRightAnalogStickValue();

        // Invert the y values to match screen space
        rawValue.y = -rawValue.y;
        filteredValue.y = -filteredValue.y;

        // Draw max stick range and dead zone range
        float const innerDeadZoneRadius = g_analogStickRangeRadius * ( isLeftStick ? settings.m_leftStickInnerDeadzone : settings.m_rightStickInnerDeadzone );
        float const outerDeadZoneRadius = g_analogStickRangeRadius * ( 1.0f - ( isLeftStick ? settings.m_leftStickOuterDeadzone : settings.m_rightStickOuterDeadzone ) );
        Float2 const analogStickCenter = position + Float2( g_analogStickRangeRadius );
        pDrawList->AddCircle( analogStickCenter, g_analogStickRangeRadius, g_controlFillColor, 20 );
        pDrawList->AddCircleFilled( analogStickCenter, outerDeadZoneRadius, g_controlFillColor, 20 );
        pDrawList->AddCircleFilled( analogStickCenter, innerDeadZoneRadius, 0xFF333333, 20 );

        // Draw raw stick position
        Float2 stickOffset = rawValue * g_analogStickRangeRadius;
        pDrawList->AddCircleFilled( analogStickCenter + stickOffset, g_analogStickPositionRadius, 0xFF0000FF, 6 );

        // Draw filtered stick position
        VectorSIMD vDirection = VectorSIMD( filteredValue ).GetNormalized2();
        stickOffset = ( filteredValue * ( outerDeadZoneRadius - innerDeadZoneRadius ) ) + ( vDirection * innerDeadZoneRadius ).ToFloat2();
        pDrawList->AddCircleFilled( analogStickCenter + stickOffset, g_analogStickPositionRadius, 0xFF00FF00, 6 );
    }

    //-------------------------------------------------------------------------

    void InputDebugView::DrawControllerState( ControllerInputDevice const& controller )
    {
        auto const& controllerState = controller.GetControllerState();

        ImDrawList* pDrawList = ImGui::GetWindowDrawList();
        Float2 const FirstRowTopLeft = ImGui::GetCursorScreenPos();
        Float2 const triggerButtonDimensions( g_buttonWidth, ( g_analogStickRangeRadius * 2 ) + ( g_buttonWidth * 2 ) + 8 );

        Float2 totalSize;
        Float2 drawPosition;

        // Left Shoulder and trigger buttons
        drawPosition = FirstRowTopLeft;
        DrawButton( pDrawList, drawPosition, g_buttonDimensions, "LB", controllerState.IsHeldDown( Input::ControllerButton::ShoulderLeft ) );
        drawPosition.y += g_buttonDimensions.y;
        DrawTriggerButton( pDrawList, drawPosition, triggerButtonDimensions, "LT", controllerState, true );

        // Left analog stick
        drawPosition = Float2( drawPosition.x + g_buttonDimensions.x + 9, FirstRowTopLeft.y );
        DrawAnalogStick( pDrawList, drawPosition, controller, true );

        // Right analog stick
        drawPosition = Float2( drawPosition.x + 26 + g_analogStickRangeRadius * 2, FirstRowTopLeft.y );
        DrawAnalogStick( pDrawList, drawPosition, controller, false );

        // Right Shoulder and trigger buttons
        drawPosition = Float2( drawPosition.x + g_analogStickRangeRadius * 2 + 9, FirstRowTopLeft.y );
        DrawButton( pDrawList, drawPosition, g_buttonDimensions, "RB", controllerState.IsHeldDown( Input::ControllerButton::ShoulderRight ) );
        drawPosition.y += g_buttonDimensions.y;
        DrawTriggerButton( pDrawList, drawPosition, triggerButtonDimensions, "RT", controllerState, false );

        totalSize.x = ( drawPosition.x + g_buttonWidth ) - FirstRowTopLeft.x;
        totalSize.y = ( g_analogStickRangeRadius * 2 ) + 8;

        //-------------------------------------------------------------------------

        Float2 const SecondRowTopLeft = Float2( FirstRowTopLeft.x, FirstRowTopLeft.y + totalSize.y );

        // D-Pad
        float const upButtonTopLeft = SecondRowTopLeft.x + ( g_buttonWidth + 9 + g_analogStickRangeRadius ) - g_buttonWidth / 2;

        drawPosition = Float2( upButtonTopLeft, SecondRowTopLeft.y );
        DrawButton( pDrawList, drawPosition, g_buttonDimensions, "Up", controllerState.IsHeldDown( Input::ControllerButton::DPadUp ) );
        drawPosition = Float2( upButtonTopLeft - g_buttonWidth, SecondRowTopLeft.y + g_buttonWidth );
        DrawButton( pDrawList, drawPosition, g_buttonDimensions, "Lt", controllerState.IsHeldDown( Input::ControllerButton::DPadLeft ) );
        drawPosition = Float2( upButtonTopLeft + g_buttonWidth, SecondRowTopLeft.y + g_buttonWidth );
        DrawButton( pDrawList, drawPosition, g_buttonDimensions, "Rt", controllerState.IsHeldDown( Input::ControllerButton::DPadRight ) );
        drawPosition = Float2( upButtonTopLeft, SecondRowTopLeft.y + g_buttonWidth + g_buttonWidth );
        DrawButton( pDrawList, drawPosition, g_buttonDimensions, "Dn", controllerState.IsHeldDown( Input::ControllerButton::DPadDown ) );

        // Face Buttons
        float const topFaceButtonTopLeft = SecondRowTopLeft.x + ( ( g_buttonWidth + g_analogStickRangeRadius ) - g_buttonWidth / 2 ) * 2 + 34 + ( g_buttonWidth * 2 );

        drawPosition = Float2( topFaceButtonTopLeft, SecondRowTopLeft.y );
        DrawButton( pDrawList, drawPosition, g_buttonDimensions, "Y", controllerState.IsHeldDown( Input::ControllerButton::FaceButtonUp ), 0xFF00FFFF );
        drawPosition = Float2( topFaceButtonTopLeft - g_buttonWidth, SecondRowTopLeft.y + g_buttonWidth );
        DrawButton( pDrawList, drawPosition, g_buttonDimensions, "X", controllerState.IsHeldDown( Input::ControllerButton::FaceButtonLeft ), 0xFFFF0000 );
        drawPosition = Float2( topFaceButtonTopLeft + g_buttonWidth, SecondRowTopLeft.y + g_buttonWidth );
        DrawButton( pDrawList, drawPosition, g_buttonDimensions, "B", controllerState.IsHeldDown( Input::ControllerButton::FaceButtonRight ), 0xFF0000FF );
        drawPosition = Float2( topFaceButtonTopLeft, SecondRowTopLeft.y + g_buttonWidth + g_buttonWidth );
        DrawButton( pDrawList, drawPosition, g_buttonDimensions, "A", controllerState.IsHeldDown( Input::ControllerButton::FaceButtonDown ), 0xFF00FF00 );

        // System Buttons
        drawPosition = Float2( SecondRowTopLeft.x + g_buttonWidth + g_analogStickRangeRadius * 2, SecondRowTopLeft.y + 10 );
        DrawButton( pDrawList, drawPosition, g_buttonDimensions, "S0", controllerState.IsHeldDown( Input::ControllerButton::System0 ) );
        drawPosition = Float2( drawPosition.x + g_buttonWidth + 4, drawPosition.y );
        DrawButton( pDrawList, drawPosition, g_buttonDimensions, "S1", controllerState.IsHeldDown( Input::ControllerButton::System1 ) );

        // Stick Buttons
        drawPosition = Float2( SecondRowTopLeft.x + g_buttonWidth + g_analogStickRangeRadius * 2, drawPosition.y + g_buttonWidth + 4 );
        DrawButton( pDrawList, drawPosition, g_buttonDimensions, "LS", controllerState.IsHeldDown( Input::ControllerButton::ThumbstickLeft ) );
        drawPosition = Float2( drawPosition.x + g_buttonWidth + 4, drawPosition.y );
        DrawButton( pDrawList, drawPosition, g_buttonDimensions, "RS", controllerState.IsHeldDown( Input::ControllerButton::ThumbstickRight ) );

        totalSize.x = ( drawPosition.x + g_buttonWidth ) - FirstRowTopLeft.x;
        totalSize.y = triggerButtonDimensions.y + g_buttonWidth + 4;

        //-------------------------------------------------------------------------
        ImGui::Dummy( totalSize );
    }

    //-------------------------------------------------------------------------

    void InputDebugView::Initialize( SystemRegistry const& systemRegistry, EntityWorld const* pWorld )
    {
        DebugView::Initialize( systemRegistry, pWorld );
        m_pInputSystem = systemRegistry.GetSystem<InputSystem>();
    }

    void InputDebugView::Shutdown()
    {
        m_pInputSystem = nullptr;
        DebugView::Shutdown();
    }

    void InputDebugView::DrawMenu( EntityWorldUpdateContext const& context )
    {
        ENGINE_ASSERT( m_pInputSystem != nullptr );

        //-------------------------------------------------------------------------

        ImGui::MenuItem( "Mouse State (TODO)" );

        ImGui::MenuItem( "Keyboard State (TODO)" );

        //-------------------------------------------------------------------------

        ImGui::Separator();

        if ( m_numControllers > 0 )
        {
            String str;
            for ( int32_t i = 0; i < m_numControllers; i++ )
            {
                str = StringFormat("Show Controller State: {}", i);
                if (ImGui::MenuItem(str.c_str()))
                {
                    for ( auto& window : m_windows )
                    {
                        if ( m_windows[i].m_typeID == g_controllerWindowTypeID && m_windows[i].m_userData == i )
                        {
                            m_windows[i].m_isOpen = true;
                        }
                    }
                }
            }
        }
        else
        {
            ImGui::Text( "No Controllers Connected" );
        }
    }

    void InputDebugView::Update( EntityWorldUpdateContext const& context )
    {
        int32_t const numControllers = m_pInputSystem->GetNumConnectedControllers();
        if ( numControllers != m_numControllers )
        {
            m_numControllers = numControllers;

            // Remove all controller windows
            for ( int32_t i = (int32_t) m_windows.size() - 1; i >= 0; i-- )
            {
                if ( m_windows[i].m_typeID == g_controllerWindowTypeID )
                {
                    m_windows.erase( m_windows.begin() + i );
                }
            }

            // Create controller windows
            String str;
            for ( int32_t i = 0; i < numControllers; i++ )
            {
                auto DrawControllerStateLambda = [this] ( EntityWorldUpdateContext const& context, bool isFocused, uint64_t userData )
                {
                    auto pControllerDevice = m_pInputSystem->GetControllerDevice( (uint32) userData );
                    DrawControllerState( *pControllerDevice );
                };

                str = StringFormat( "Controller State: Controller {}", i );
                m_windows.emplace_back( str.c_str(), DrawControllerStateLambda );
                m_windows.back().m_typeID = g_controllerWindowTypeID;
                m_windows.back().m_userData = i;
            }
        }
    }
}
#endif