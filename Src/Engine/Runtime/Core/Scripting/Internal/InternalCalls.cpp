#include "InternalCalls.h"

#include "Runtime/Core/Profiler/ProfilerCPU.h"
#include "Runtime/Core/Profiler/ProfilerGPU.h"
#include "Runtime/Core/Thread/ThreadLocal.h"
#include "Runtime/Core/Types/Collections/ChunkedArray.h"
#include "Runtime/Core/Thread/Threading.h"
#include "Runtime/Core/Scripting/ScriptingObject.h"
#include "Runtime/Core/Scripting/ManagedCLR/CLRException.h"
#include "Runtime/Core/Scripting/ManagedCLR/CLRUtils.h"

namespace SE
{
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
		//const String objName = obj ? obj->ToString() : String::Empty;

		/*StringView category;
		CLRUtils::ToString(categoryObj, msg);*/

		// Send event
		// TODO: maybe option for build to threat warnings and errors as fatal errors?
		//const String logMessage = String::Format(TEXT("Debug:{1} {2}"), objName, *
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
		ex.Log(Log::Severity::Warning, objName.GetText());
	}

	namespace
	{
#if SE_PROFILER
		List<int32, InlinedAllocation<32>> ManagedEventsGPU;
#if TRACY_ENABLE && !PROFILE_CPU_USE_TRANSIENT_DATA
		CriticalSection ManagedSourceLocationsLocker;

		struct Location
		{
			String Name;
			StringAnsi NameAnsi;
			tracy::SourceLocationData SrcLocation;
		};

		ChunkedArray<Location, 256> ManagedSourceLocations;
		Threading::ThreadLocal<uint32> ManagedEventsCount;
#endif
#endif
	}

	DEFINE_INTERNAL_CALL(void) Internal_BeginEvent(CLRString* nameObj)
	{
#if SE_PROFILER
		StringView name;
		CLRUtils::ToString(nameObj, name);
		ProfilerCPU::BeginEvent(*name);
#if TRACY_ENABLE
#if PROFILE_CPU_USE_TRANSIENT_DATA
		tracy::ScopedZone::Begin(__LINE__, __FILE__, strlen( __FILE__ ), __FUNCTION__, strlen( __FUNCTION__ ), name.Get(), name.Length() );
#else
		Threading::ScopeLock lock(ManagedSourceLocationsLocker);
		tracy::SourceLocationData* srcLoc = nullptr;
		for (auto e = ManagedSourceLocations.Begin(); e.IsNotEnd(); ++e)
		{
			if (name == e->Name)
			{
				srcLoc = &e->SrcLocation;
				break;
			}
		}
		if (!srcLoc)
		{
			auto& e = ManagedSourceLocations.AddOne();
			e.Name = name;
			e.NameAnsi = name.Get();
			srcLoc = &e.SrcLocation;
			srcLoc->name = e.NameAnsi.Get();
			srcLoc->function = nullptr;
			srcLoc->file = nullptr;
			srcLoc->line = 0;
			srcLoc->color = 0;
		}
		//static constexpr tracy::SourceLocationData tracySrcLoc{ nullptr, __FUNCTION__, __FILE__, (uint32_t)__LINE__, 0 };
		if (tracy::ScopedZone::Begin(srcLoc))
			ManagedEventsCount.Get()++;
#endif
#endif
#endif
	}

	DEFINE_INTERNAL_CALL(void) Internal_EndEvent()
	{
#if SE_PROFILER
#if TRACY_ENABLE
		uint32& tracyActive = ManagedEventsCount.Get();
		if (tracyActive > 0)
		{
			tracyActive--;
			tracy::ScopedZone::End();
		}
#endif
		ProfilerCPU::EndEvent();
#endif
	}

	DEFINE_INTERNAL_CALL(void) Internal_BeginEventGPU(CLRString* nameObj)
	{
#if SE_PROFILER
		const StringView nameChars = CLRCore::String::GetChars(nameObj);
		const auto index = ProfilerGPU::BeginEvent(nameChars.Get());
		ManagedEventsGPU.Push(index);
#endif
	}

	DEFINE_INTERNAL_CALL(void) Internal_EndEventGPU()
	{
#if SE_PROFILER
		const auto index = ManagedEventsGPU.Pop();
		ProfilerGPU::EndEvent(index);
#endif
	}

	void RegisterInternalCalls()
	{
		ADD_INTERNAL_CALL("SE.Utils::MemoryCopy", &Internal_MemoryCopy);
		ADD_INTERNAL_CALL("SE.Utils::MemoryClear", &PlatformInternal_MemoryClear);
		ADD_INTERNAL_CALL("SE.Utils::MemoryCompare", &PlatformInternal_MemoryCompare);

		ADD_INTERNAL_CALL("SE.LogHandler::Internal_LogWrite", &Internal_LogWrite);
		ADD_INTERNAL_CALL("SE.LogHandler::Internal_Log", &Internal_Log);
		ADD_INTERNAL_CALL("SE.LogHandler::Internal_LogException", &Internal_LogException);
	}
}
