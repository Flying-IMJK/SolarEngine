
#pragma once

#if PLATFORM_WIN32

#include "Core/Platform/Base/PlatformBase.h"
#if _MSC_VER <= 1900
#include <intrin.h>
#else
#include <intrin0.h>
#endif

namespace SE
{
	extern "C" __declspec(dllimport) unsigned long __stdcall GetCurrentThreadId(void);

	/// <summary>
	/// The Win32 platform implementation and application management utilities.
	/// </summary>
	class SE_API_CORE Win32Platform : public PlatformBase
	{
	public:

		// [PlatformBase]
		static bool Init();
		static void Exit();
		static void MemoryBarrier();
		static int64 AtomicExchange(int64 volatile* dst, int64 exchange)
		{
#if WIN32
			return ::_interlockedexchange64(dst, exchange);
#else
			return ::_InterlockedExchange64(dst, exchange);
#endif
		}
		static int32 AtomicCompareExchange(int32 volatile* dst, int32 exchange, int32 comperand)
		{
			return ::_InterlockedCompareExchange((long volatile*)dst, (long)exchange, (long)comperand);
		}
		static int64 AtomicCompareExchange(int64 volatile* dst, int64 exchange, int64 comperand)
		{
			return ::_InterlockedCompareExchange64(dst, exchange, comperand);
		}

		static int64 AtomicIncrement(volatile int64* dst)
		{
#if WIN32
			return ::_interlockedincrement64(dst);
#else
			return ::_InterlockedExchangeAdd64(dst, 1);
#endif
		}
		static int64 AtomicDecrement(int64 volatile* dst)
		{
#if WIN32
			return ::_interlockeddecrement64(dst);
#else
			return ::_InterlockedExchangeAdd64(dst, -1);
#endif
		}
		static int64 AtomicAdd(int64 volatile* dst, int64 value)
		{
#if WIN32
			return ::_interlockedexchangeadd64(dst, value);
#else
			return ::_InterlockedExchangeAdd64(dst, value);
#endif
		}
		static int32 AtomicRead(int32 const volatile* dst)
		{
			return (int32)::_InterlockedCompareExchange((long volatile*)dst, 0, 0);
		}
		static int64 AtomicRead(int64 const volatile* dst)
		{
			return ::_InterlockedCompareExchange64((int64 volatile*)dst, 0, 0);
		}
		static void AtomicStore(int32 volatile* dst, int32 value)
		{
			::_InterlockedExchange((long volatile*)dst, value);
		}
		static void AtomicStore(int64 volatile* dst, int64 value)
		{
#if WIN32
			::_interlockedexchange64(dst, value);

#else
			::_InterlockedExchange64(dst, value);
#endif
		}
		static void Prefetch(void const* ptr);
		static void* Allocate(uint64 size, uint64 alignment);
		static void Free(void* ptr);
		static void* AllocatePages(uint64 numPages, uint64 pageSize);
		static void FreePages(void* ptr);
		static bool Is64BitPlatform();
		static CPUInfo GetCPUInfo();
		static int32 GetCacheLineSize();
		static MemoryStats GetMemoryStats();
		static ProcessMemoryStats GetProcessMemoryStats();
		static uint64 GetCurrentProcessId();
		static uint64 GetCurrentThreadID()
		{
			return GetCurrentThreadId();
		}
		static void SetThreadPriority(ThreadPriority priority);
		static void SetThreadAffinityMask(uint64 affinityMask);
		static void Sleep(int32 milliseconds);
		static double GetTimeSeconds();
		static uint64 GetTimeCycles();
		static uint64 GetClockFrequency();
		static void GetSystemTime(int32& year, int32& month, int32& dayOfWeek, int32& day, int32& hour, int32& minute, int32& second, int32& millisecond);
		static void GetUTCTime(int32& year, int32& month, int32& dayOfWeek, int32& day, int32& hour, int32& minute, int32& second, int32& millisecond);
		static void CreateGuid(UID& result);
		static String GetMainDirectory();
		static String GetExecutableFilePath();
		static UID GetUniqueDeviceId();
		static String GetWorkingDirectory();
		static bool SetWorkingDirectory(const String& path);
		static void FreeLibrary(void* handle);
		static void* GetProcAddress(void* handle, const char* symbol);
	};
}

#endif
