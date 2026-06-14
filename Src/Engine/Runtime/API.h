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

namespace SE
{
	class BinaryModule;
	extern "C" SE_API_RUNTIME BinaryModule* GetBinaryModuleSERuntime();

#ifdef SE_EDITOR

	extern "C" SE_API_RUNTIME BinaryModule* GetBinaryModuleSEEditor();

#endif

}
