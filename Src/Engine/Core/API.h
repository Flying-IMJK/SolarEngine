#pragma once

#include "Core/Compiler.h"
//-------------------------------------------------------------------------
// DLL support
//-------------------------------------------------------------------------
#ifdef SE_DLL_CORE
#define SE_API_CORE DLLEXPORT
#define SE_API_CORE_TEMPLATE DLLEXPORT
#else
#define SE_API_CORE DLLIMPORT
#define SE_API_CORE_TEMPLATE
#endif