#pragma once

#include "Core/API.h"
#include "Core/TypeSystem/IType.h"
#include "Core/TypeSystem/Types.h"

//-------------------------------------------------------------------------

namespace SE
{
    class SE_API_CORE CoreModule
    {
        ENGINE_REFLECT_MODULE;

    public:
        bool Initialize();
        void Shutdown();
    };
}