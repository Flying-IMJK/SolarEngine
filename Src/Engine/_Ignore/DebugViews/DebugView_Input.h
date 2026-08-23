#pragma once

#include "DebugView.h"
#include "Core/Input/InputDevices/InputDevice_Controller.h"

//-------------------------------------------------------------------------

#if ENGINE_DEVELOPMENT_TOOLS
namespace SGE::Input
{
    class ControllerInputDevice;
    class InputSystem;

    //-------------------------------------------------------------------------

    class InputDebugView : public DebugView
    {
        SE_CLASS( InputDebugView, DebugView);

    public:

        InputDebugView() : DebugView( "Engine/Input" ) {}

    private:

        virtual void Initialize( SystemRegistry const& systemRegistry, EntityWorld const* pWorld ) override;
        virtual void Shutdown() override;
        virtual void DrawMenu( EntityWorldUpdateContext const& context ) override;
        virtual void Update( EntityWorldUpdateContext const& context ) override;

        void DrawControllerState( ControllerInputDevice const& controllerState );

    private:

        InputSystem*                                  m_pInputSystem = nullptr;
        int32                                         m_numControllers = 0;
    };
}
#endif