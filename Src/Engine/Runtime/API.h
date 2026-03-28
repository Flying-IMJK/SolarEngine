#pragma once

#include "Core/Compiler.h"
//-------------------------------------------------------------------------
// DLL support
//-------------------------------------------------------------------------
#ifdef SE_DLL_RUNTIME
#define SE_API_RUNTIME DLLEXPORT
#else
#define SE_API_RUNTIME DLLIMPORT
#endif