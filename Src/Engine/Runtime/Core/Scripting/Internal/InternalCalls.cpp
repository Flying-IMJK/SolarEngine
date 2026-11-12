#include "InternalCalls.h"

#include "Runtime/Core/Logging/LoggingSystem.h"
#include "Runtime/Core/Thread/ThreadLocal.h"
#include "Runtime/Core/Thread/Threading.h"
#include "Runtime/Core/Scripting/ScriptingObject.h"
#include "Runtime/Core/Scripting/ManagedCLR/CLRException.h"
#include "Runtime/Core/Scripting/ManagedCLR/CLRUtils.h"

namespace SE
{
	using NativeLogCallback = void (*)(Log::Severity, const Char*);
	static NativeLogCallback s_NativeLogCallback = nullptr;

	// Connect the managed log handler to the native log system without replacing other listeners.
	DEFINE_INTERNAL_CALL(void) Internal_SetLogCallback(NativeLogCallback callback)
	{
		if (s_NativeLogCallback)
			Log::System::g_LogCallback.Unbind(s_NativeLogCallback);
		s_NativeLogCallback = callback;
		if (callback)
			Log::System::g_LogCallback.Bind(callback);
	}

	DEFINE_INTERNAL_CALL(void) Internal_MemoryCopy(void* dst, const void* src, uint64 size)
	{
		Platform::MemoryCopy(dst, src, size);
	}

	DEFINE_INTERNAL_CALL(void) Internal_MemoryClear(void* dst, uint64 size)
	{
		Platform::MemoryClear(dst, size);
	}

	DEFINE_INTERNAL_CALL(int32) Internal_MemoryCompare(const void* buf1, const void* buf2, uint64 size)
	{
		return Platform::MemoryCompare(buf1, buf2, size);
	}

	DEFINE_INTERNAL_CALL(void) Internal_LogWrite(Log::Severity level/*, CLRString* categoryObj*/, CLRString* msgObj)
	{
		StringView msg;
		CLRUtils::ToString(msgObj, msg);
		/*StringView category;
		CLRUtils::ToString(categoryObj, msg);*/

		Log::AddEntry(level, SE_TEXT("Scripting"), __FILE__, __LINE__, msg.Get());
	}

	DEFINE_INTERNAL_CALL(void) Internal_Log(Log::Severity level/*, CLRString* categoryObj*/, CLRString* msgObj, ScriptingObject* obj, CLRString* stackTrace)
	{
		if (msgObj == nullptr)
		{
			return;
		}

		// Get info
		StringView msg;
		CLRUtils::ToString(msgObj, msg);

		Log::AddEntry(level, SE_TEXT("Scripting"),  __FILE__, __LINE__, msg.Get());
	}

	DEFINE_INTERNAL_CALL(void) Internal_LogException(CLRObject* exception, ScriptingObject* obj)
	{

		if (exception == nullptr)
			return;

		// Get info
		CLRException ex(exception);
		const String objName = obj ? obj->ToString() : String::Empty;

		// Print exception including inner exceptions
		// TODO: maybe option for build to threat warnings and errors as fatal errors?
		ex.Log(Log::Severity::Error, objName.GetText());
	}


	void RegisterInternalCalls()
	{
		ADD_INTERNAL_CALL("SE.Utils::MemoryCopy", &Internal_MemoryCopy);
		ADD_INTERNAL_CALL("SE.Utils::MemoryClear", &PlatformInternal_MemoryClear);
		ADD_INTERNAL_CALL("SE.Utils::MemoryCompare", &PlatformInternal_MemoryCompare);

		ADD_INTERNAL_CALL("SE.LogHandler::Internal_LogWrite", &Internal_LogWrite);
		ADD_INTERNAL_CALL("SE.LogHandler::Internal_Log", &Internal_Log);
		ADD_INTERNAL_CALL("SE.LogHandler::Internal_LogException", &Internal_LogException);
		ADD_INTERNAL_CALL("SE.LogHandler::Internal_SetLogCallback", &Internal_SetLogCallback);
	}
}
