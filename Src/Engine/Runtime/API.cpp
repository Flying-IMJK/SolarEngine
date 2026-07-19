
#include "API.h"
#include "Runtime/Core/Scripting/Binary/NativeBinaryModule.h"

namespace SE
{
	BinaryModule* GetBinaryModuleSERuntime()
	{
		static NativeBinaryModule module("SERuntimeCSharp");
		return &module;
	}

#ifdef SE_EDITOR

	BinaryModule* GetBinaryModuleSEEditor()
	{
		static NativeBinaryModule module("SE.EditorCSharp");
		return &module;
	}
#endif

}
