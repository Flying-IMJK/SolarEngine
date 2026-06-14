#pragma once

#include "Core/Logging/Logging.h"
#include "Runtime/Scripting/ScriptingType.h"
#include "Runtime/Scripting/ManagedCLR/CLRTypes.h"

#if defined(__clang__)
// Helper utility to override vtable entry with automatic restore
// See BindingsGenerator.Cpp.cs that generates virtuall method wrappers for scripting to properly call overriden base method
struct SE_API_RUNTIME VTableFunctionInjector
{
    void** VTableAddr;
    void* OriginalValue;

    VTableFunctionInjector(void* object, void* originalFunc, void* func)
    {
        void** vtable = *(void***)object;
        const int32 vtableIndex = SE::GetVTableIndex(vtable, 200, originalFunc);
        VTableAddr = vtable + vtableIndex;
        OriginalValue = *VTableAddr;
        *VTableAddr = func;
    }

    ~VTableFunctionInjector()
    {
        *VTableAddr = OriginalValue;
    }
};
#elif defined(_MSC_VER)
#define MSVC_FUNC_EXPORT(name) __pragma(comment(linker, "/EXPORT:" #name "=" __FUNCDNAME__))
#endif


#define ADD_INTERNAL_CALL(fullName, method)
#define DEFINE_INTERNAL_CALL(returnType) extern "C" DLLEXPORT returnType


#if BUILD_RELEASE && 0

// Using invalid handle will crash engine in Release build
#define INTERNAL_CALL_CHECK(obj)
#define INTERNAL_CALL_CHECK_EXP(expression)
#define INTERNAL_CALL_CHECK_RETURN(obj, defaultValue)
#define INTERNAL_CALL_CHECK_EXP_RETURN(expression, defaultValue)

#else

// Use additional checks in debug/development builds
#define INTERNAL_CALL_CHECK(obj) \
	if (obj == nullptr) \
	{ \
		DebugLog::ThrowNullReference(); \
		return; \
	}
#define INTERNAL_CALL_CHECK_EXP(expression) \
	if (expression) \
	{ \
		DebugLog::ThrowNullReference(); \
		return; \
	}
#define INTERNAL_CALL_CHECK_RETURN(obj, defaultValue) \
	if (obj == nullptr) \
	{ \
		DebugLog::ThrowNullReference(); \
		return defaultValue; \
	}
#define INTERNAL_CALL_CHECK_EXP_RETURN(expression, defaultValue) \
	if (expression) \
	{ \
		DebugLog::ThrowNullReference(); \
		return defaultValue; \
	}

#endif

