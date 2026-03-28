#pragma once

#include "Runtime/API.h"
#include "Runtime/SGUI/GUILayout.h"
#include "Core/TypeSystem/IType.h"
#include "Core/TypeSystem/Types.h"
#include "Core/Systems.h"

namespace SE
{
    class SE_API_RUNTIME RuntimeModule
    {
        ENGINE_REFLECT_MODULE;
        
        static void GetListOfAllRequiredModuleResources(List<ResID>& outResourceIDs );
    private:

        bool                                            m_ModuleInitialized = false;
    };
    
} // namespace SE