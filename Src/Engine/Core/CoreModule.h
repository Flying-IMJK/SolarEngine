#pragma once

#include "API.h"
#include "TypeSystem/IType.h"


//-------------------------------------------------------------------------

namespace SE
{
    class SE_API_CORE CoreModule
    {
    public:
        bool Initialize();
        void Shutdown();
    };
}