#ifdef _WIN32
#pragma once

#include "Runtime/API.h"
#include "Core/Types/Variable.h"

//-------------------------------------------------------------------------

struct HWND__;

//-------------------------------------------------------------------------

#ifdef SE_DEVELOPMENT
namespace SE::SGUI::Platform
{
    // Returns 0 when the message isnt handled, used to embed into another wnd proc
    SE_API_RUNTIME intptr WindowMessageProcessor(HWND__* hWnd, uint32 message, uintptr wParam, intptr lParam );
}
#endif
#endif