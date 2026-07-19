#pragma once

#include "Runtime/Core/Platform/Compiler.h"

//-------------------------------------------------------------------------
// DLL support
//-------------------------------------------------------------------------
#ifdef SE_DLL_EDITOR
#define SE_API_EDITOR DLLEXPORT
#else
#define SE_API_EDITOR DLLIMPORT
#endif