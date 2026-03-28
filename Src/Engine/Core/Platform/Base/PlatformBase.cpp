
#include "Core/Platform/Platform.h"
#include "Core/Platform/CPUInfo.h"
#include "Core/Platform/CreateProcessSettings.h"
#include "Core/Platform/User.h"
#include "Core/Types/DateTime.h"
#include "Core/Types/TimeSpan.h"
#include "Core/Types/UID.h"
#include "Core/Types/Collections/Dictionary.h"
#include "Core/Math/Rectangle.h"
#include "Core/Types/Delegate.h"
#include "Core/Types/Strings/StringConverter.h"

#if SE_PROFILER
#include "Core/Profiler/ProfilerCPU.h"
#endif

#include <iostream>


namespace SE
{
	// Check types sizes
	static_assert(sizeof(int8) == 1, "Invalid int8 type size.");
	static_assert(sizeof(int16) == 2, "Invalid int16 type size.");
	static_assert(sizeof(int32) == 4, "Invalid int32 type size.");
	static_assert(sizeof(int64) == 8, "Invalid int64 type size.");
	static_assert(sizeof(uint8) == 1, "Invalid uint8 type size.");
	static_assert(sizeof(uint16) == 2, "Invalid uint16 type size.");
	static_assert(sizeof(uint32) == 4, "Invalid uint32 type size.");
	static_assert(sizeof(uint64) == 8, "Invalid uint64 type size.");
	static_assert(sizeof(Char) == 2, "Invalid Char type size.");
	static_assert(sizeof(char) == 1, "Invalid char type size.");
	static_assert(sizeof(bool) == 1, "Invalid bool type size.");
	static_assert(sizeof(float) == 4, "Invalid float type size.");
	static_assert(sizeof(double) == 8, "Invalid double type size.");

// Check configuration
	static_assert((PLATFORM_THREADS_LIMIT & (PLATFORM_THREADS_LIMIT - 1)) == 0, "Threads limit must be power of two.");
	static_assert(PLATFORM_THREADS_LIMIT % 4 == 0, "Threads limit must be multiple of 4.");

	float PlatformBase::CustomDpiScale = 1.0f;
	List<User*, FixedAllocation<8>> PlatformBase::Users;
	Delegate<User*> PlatformBase::UserAdded;
	Delegate<User*> PlatformBase::UserRemoved;

	const Char* ToString(ScreenOrientationType value)
	{
		switch (value)
		{
		case ScreenOrientationType::Unknown:
			return SE_TEXT("Unknown");
		case ScreenOrientationType::Portrait:
			return SE_TEXT("Portrait");
		case ScreenOrientationType::PortraitUpsideDown:
			return SE_TEXT("PortraitUpsideDown");
		case ScreenOrientationType::LandscapeLeft:
			return SE_TEXT("LandscapeLeft");
		case ScreenOrientationType::LandscapeRight:
			return SE_TEXT("LandscapeRight");
		default:
			return SE_TEXT("");
		}
	}

	const Char* ToString(ThreadPriority value)
	{
		switch (value)
		{
		case ThreadPriority::Normal:
			return SE_TEXT("Normal");
		case ThreadPriority::AboveNormal:
			return SE_TEXT("AboveNormal");
		case ThreadPriority::BelowNormal:
			return SE_TEXT("BelowNormal");
		case ThreadPriority::Highest:
			return SE_TEXT("Highest");
		case ThreadPriority::Lowest:
			return SE_TEXT("Lowest");
		default:
			return SE_TEXT("");
		}
	}

	UserBase::UserBase(const String& name)
		: _name(name)
	{
	}

	String UserBase::GetName() const
	{
		return _name;
	}


	bool PlatformBase::Init()
	{
#if BUILD_DEBUG
		// Validate atomic and interlocked operations
		int64 data = 0;
		Platform::AtomicStore(&data, 11);
		ASSERT(Platform::AtomicRead(&data) == 11);
		ASSERT(Platform::InterlockedAdd(&data, 2) == 11);
		ASSERT(Platform::AtomicRead(&data) == 13);
		ASSERT(Platform::InterlockedIncrement(&data) == 14);
		ASSERT(Platform::AtomicRead(&data) == 14);
		ASSERT(Platform::InterlockedDecrement(&data) == 13);
		ASSERT(Platform::AtomicRead(&data) == 13);
		ASSERT(Platform::InterlockedExchange(&data, 10) == 13);
		ASSERT(Platform::AtomicRead(&data) == 10);
		ASSERT(Platform::InterlockedCompareExchange(&data, 11, 0) == 10);
		ASSERT(Platform::AtomicRead(&data) == 10);
		ASSERT(Platform::InterlockedCompareExchange(&data, 11, 10) == 10);
		ASSERT(Platform::AtomicRead(&data) == 11);
#endif

//		srand((unsigned int)Platform::GetTimeCycles());

		return false;
	}

	void PlatformBase::LogInfo()
	{
		// LOG(Info, "Computer name: {0}", Platform::GetComputerName());
		// LOG(Info, "User name: {0}", Platform::GetUserName());

		/*const CPUInfo cpuInfo = Platform::GetCPUInfo();
		LOG(Info, "CPU package count: {0}, Core count: {1}, Logical processors: {2}", cpuInfo.ProcessorPackageCount, cpuInfo.ProcessorCoreCount, cpuInfo.LogicalProcessorCount);
		LOG(Info, "CPU Page size: {0}, cache line size: {1} bytes", Utilities::BytesToText(cpuInfo.PageSize), cpuInfo.CacheLineSize);
		LOG(Info,
			"L1 cache: {0}, L2 cache: {1}, L3 cache: {2}",
			Utilities::BytesToText(cpuInfo.L1CacheSize),
			Utilities::BytesToText(cpuInfo.L2CacheSize),
			Utilities::BytesToText(cpuInfo.L3CacheSize));
		LOG(Info, "Clock speed: {0}", Utilities::HertzToText(cpuInfo.ClockSpeed));

		const MemoryStats memStats = Platform::GetMemoryStats();
		LOG(Info,
			"Physical Memory: {0} total, {1} used ({2}%)",
			Utilities::BytesToText(memStats.TotalPhysicalMemory),
			Utilities::BytesToText(memStats.UsedPhysicalMemory),
			Utilities::RoundTo2DecimalPlaces((float)memStats.UsedPhysicalMemory * 100.0f / (float)memStats.TotalPhysicalMemory));
		LOG(Info,
			"Virtual Memory: {0} total, {1} used ({2}%)",
			Utilities::BytesToText(memStats.TotalVirtualMemory),
			Utilities::BytesToText(memStats.UsedVirtualMemory),
			Utilities::RoundTo2DecimalPlaces((float)memStats.UsedVirtualMemory * 100.0f / (float)memStats.TotalVirtualMemory));

		LOG(Info, "Main thread id: 0x{0:x}, Process id: {1}", Globals::MainThreadID, Platform::GetCurrentProcessId());
		LOG(Info, "Desktop size: {0}", Platform::GetDesktopSize());
		LOG(Info, "Virtual Desktop size: {0}", Platform::GetVirtualDesktopBounds());
		LOG(Info, "Screen DPI: {0}", Platform::GetDpi());*/
	}

	void PlatformBase::BeforeRun()
	{
	}

	void PlatformBase::Tick()
	{
	}

	void PlatformBase::BeforeExit()
	{
	}

	void PlatformBase::Exit()
	{
	}

	void PlatformBase::MemoryCopy(void* dst, const void* src, uint64 size)
	{
		memcpy(dst, src, size);
	}

	void PlatformBase::MemorySet(void* dst, uint64 size, int32 value)
	{
		memset(dst, value, size);
	}

	void PlatformBase::MemoryClear(void* dst, uint64 size)
	{
		memset(dst, 0, size);
	}

	int32 PlatformBase::MemoryCompare(const void* buf1, const void* buf2, uint64 size)
	{
		return memcmp(buf1, buf2, size);
	}

#if SE_PROFILER

	void PlatformBase::OnMemoryAlloc(void* ptr, uint64 size)
	{
		if (!ptr)
			return;

#if TRACY_ENABLE
		// Track memory allocation in Tracy
		//tracy::Profiler::MemAlloc(ptr, (size_t)size, false);
		tracy::Profiler::MemAllocCallstack(ptr, (size_t)size, 12, false);
#endif

		// Register allocation during the current CPU event
		auto thread = ProfilerCPU::GetCurrentThread();
		if (thread != nullptr && thread->Buffer.GetCount() != 0)
		{
			auto& activeEvent = thread->Buffer.Last().Event();
			if (activeEvent.End < Math::ZeroTolerance)
			{
				activeEvent.NativeMemoryAllocation += (int32)size;
			}
		}
	}

	void PlatformBase::OnMemoryFree(void* ptr)
	{
		if (!ptr)
			return;

#if TRACY_ENABLE
		// Track memory allocation in Tracy
		tracy::Profiler::MemFree(ptr, false);
#endif
	}

#endif

	void* PlatformBase::AllocatePages(uint64 numPages, uint64 pageSize)
	{
		// Fallback to the default memory allocation
		const uint64 numBytes = numPages * pageSize;
		return Platform::Allocate(numBytes, pageSize);
	}

	void PlatformBase::FreePages(void* ptr)
	{
		// Fallback to free
		Platform::Free(ptr);
	}

	PlatformType PlatformBase::GetPlatformType()
	{
		return PLATFORM_TYPE;
	}

	bool PlatformBase::Is64BitApp()
	{
#if PLATFORM_64BITS
		return true;
#else
		return false;
#endif
	}

	void PlatformBase::SetHighDpiAwarenessEnabled(bool enable)
	{
	}

	/*BatteryInfo PlatformBase::GetBatteryInfo()
	{
		return BatteryInfo();
	}*/

	int32 PlatformBase::GetDpi()
	{
		return 96;
	}

	float PlatformBase::GetDpiScale()
	{
		return CustomDpiScale * (float)Platform::GetDpi() / 96.0f;
	}

	ScreenOrientationType PlatformBase::GetScreenOrientationType()
	{
		return ScreenOrientationType::Unknown;
	}

	String PlatformBase::GetUserName()
	{
		return Users.Count() != 0 ? Users[0]->GetName() : String::Empty;
	}

	bool PlatformBase::GetIsPaused()
	{
		return false;
	}

	void PlatformBase::CreateUUID(UID& result)
	{
		static uint16 guidCounter = 0;
		static DateTime guidStartTime;
		static double guidStartSeconds;

		DateTime estimatedCurrentDateTime;
		if (guidCounter == 0)
		{
			guidStartTime = DateTime::Now();
			guidStartSeconds = Platform::GetTimeSeconds();
			estimatedCurrentDateTime = guidStartTime;
		}
		else
		{
			const TimeSpan elapsedTime = TimeSpan::FromSeconds(Platform::GetTimeSeconds() - guidStartSeconds);
			estimatedCurrentDateTime = guidStartTime + elapsedTime;
		}

		const uint16 sequentialThing = static_cast<uint32>(guidCounter++);
		const uint16 randomThing = rand() & 0xFFFF;
		const uint32 dateThingHigh = estimatedCurrentDateTime.Ticks >> 32;
		const uint32 dateThingLow = estimatedCurrentDateTime.Ticks & 0xffffffff;
		const uint32 cyclesThing = Platform::GetTimeCycles() & 0xffffffff;

		result = UID(dateThingHigh, randomThing | (sequentialThing << 16), cyclesThing, dateThingLow);
	}


	void PlatformBase::Fatal(const Char* msg, void* context)
	{
		// Check if is already during fatal state
/*		if (Globals::FatalErrorOccurred)
		{
			// Just send one more error to the log and back
			LOG(Error, "Error after fatal error: {0}", msg);
			return;
		}

		// Set flags
		Globals::FatalErrorOccurred = true;
		Globals::IsRequestingExit = true;
		Globals::ExitCode = -1;

		// Collect crash info (platform-dependant implementation that might collect stack trace and/or create memory dump)
		{
			// Log separation for crash info
			Log::Logger::WriteFloor();
			LOG(Error, "");
			LOG(Error, "Critical error! Reason: {0}", msg);
			LOG(Error, "");

			// Log stack trace
			const auto stackFrames = Platform::GetStackFrames(context ? 0 : 1, 60, context);
			if (stackFrames.HasItems())
			{
				LOG(Error, "Stack trace:");
				for (const auto& frame : stackFrames)
				{
					// Remove any path from the module name
					int32 num = StringUtils::Length(frame.ModuleName);
					while (num > 0 && frame.ModuleName[num - 1] != '\\' && frame.ModuleName[num - 1] != '/' && frame.ModuleName[num - 1] != ':')
						num--;
					StringAsUTF16<ARRAY_COUNT(StackFrame::ModuleName)> moduleName(frame.ModuleName + num);
					num = moduleName.Length();
					if (num != 0 && num < ARRAY_COUNT(StackFrame::ModuleName) - 2)
					{
						// Append separator between module name and the function name
						((Char*)moduleName.Get())[num++] = '!';
						((Char*)moduleName.Get())[num] = 0;
					}

					StringAsUTF16<ARRAY_COUNT(StackFrame::FunctionName)> functionName(frame.FunctionName);
					if (StringUtils::Length(frame.FileName) != 0)
					{
						StringAsUTF16<ARRAY_COUNT(StackFrame::FileName)> fileName(frame.FileName);
						LOG(Error, "    at {0}{1}() in {2}:line {3}", moduleName.Get(), functionName.Get(), fileName.Get(), frame.LineNumber);
					}
					else if (StringUtils::Length(frame.FunctionName) != 0)
					{
						LOG(Error, "    at {0}{1}()", moduleName.Get(), functionName.Get());
					}
					else if (StringUtils::Length(frame.ModuleName) != 0)
					{
						LOG(Error, "    at {0}0x{1:x}", moduleName.Get(), (uint64)frame.ProgramCounter);
					}
					else
					{
						LOG(Error, "    at 0x{0:x}", (uint64)frame.ProgramCounter);
					}
				}
				LOG(Error, "");
			}

			// Log process memory stats
			{
				const MemoryStats memoryStats = Platform::GetMemoryStats();
				LOG(Error, "Used Physical Memory: {0} ({1}%)", Utilities::BytesToText(memoryStats.UsedPhysicalMemory), (int32)(100 * memoryStats.UsedPhysicalMemory / memoryStats.TotalPhysicalMemory));
				LOG(Error, "Used Virtual Memory: {0} ({1}%)", Utilities::BytesToText(memoryStats.UsedVirtualMemory), (int32)(100 * memoryStats.UsedVirtualMemory / memoryStats.TotalVirtualMemory));
				const ProcessMemoryStats processMemoryStats = Platform::GetProcessMemoryStats();
				LOG(Error, "Process Used Physical Memory: {0}", Utilities::BytesToText(processMemoryStats.UsedPhysicalMemory));
				LOG(Error, "Process Used Virtual Memory: {0}", Utilities::BytesToText(processMemoryStats.UsedVirtualMemory));
			}
		}
		if (Log::Logger::LogFilePath.HasChars())
		{
			// Create separate folder with crash info
			const String crashDataFolder = String(StringUtils::GetDirectoryName(Log::Logger::LogFilePath)) / TEXT("Crash_") + StringUtils::GetFileNameWithoutExtension(Log::Logger::LogFilePath).Substring(4);
			FileSystem::CreateDirectory(crashDataFolder);

			// Capture the platform-dependant crash info (eg. memory dump)
			Platform::CollectCrashData(crashDataFolder, context);

			// Capture the original log file
			LOG(Error, "");
			Log::Logger::WriteFloor();
			LOG_FLUSH();
			FileSystem::CopyFile(crashDataFolder / TEXT("Log.txt"), Log::Logger::LogFilePath);

			LOG(Error, "Crash info collected.");
			Log::Logger::WriteFloor();
		}

		// Show error message
		Error(msg);

		// Only main thread can call exit directly
		if (IsInMainThread())
		{
			Engine::Exit(-1);
		}*/
	}

	void PlatformBase::Error(const Char* msg)
	{
		std::cout << "Error: " << msg << std::endl;
/*#if PLATFORM_HAS_HEADLESS_MODE
		if (CommandLine::Options.Headless)
		{
#if PLATFORM_TEXT_IS_CHAR16
			StringAnsi ansi(msg);
			ansi += PLATFORM_LINE_TERMINATOR;
			printf("Error: %s\n", ansi.Get());
#else
			std::cout << "Error: " << msg << std::endl;
#endif
		}
		else
#endif
		{
			MessageBox::Show(nullptr, msg, TEXT("Error"), MessageBoxButtons::OK, MessageBoxIcon::Error);
		}*/
	}

	void PlatformBase::Warning(const Char* msg)
	{
		std::cout << "Warning: " << msg << std::endl;
/*#if PLATFORM_HAS_HEADLESS_MODE
		if (CommandLine::Options.Headless)
		{
			std::cout << "Warning: " << msg << std::endl;
		}
		else
#endif
		{
			MessageBox::Show(nullptr, msg, TEXT("Warning"), MessageBoxButtons::OK, MessageBoxIcon::Warning);
		}*/
	}

	void PlatformBase::Info(const Char* msg)
	{
		std::cout << "Info: " << msg << std::endl;
/*#if PLATFORM_HAS_HEADLESS_MODE
		if (CommandLine::Options.Headless)
		{
			std::cout << "Info: " << msg << std::endl;
		}
		else
#endif
		{
			MessageBox::Show(nullptr, msg, TEXT("Info"), MessageBoxButtons::OK, MessageBoxIcon::Information);
		}*/
	}

	void PlatformBase::Fatal(const StringView& msg)
	{
		Fatal(*msg);
	}

	void PlatformBase::Error(const StringView& msg)
	{
		Error(*msg);
	}

	void PlatformBase::Warning(const StringView& msg)
	{
		Warning(*msg);
	}

	void PlatformBase::Info(const StringView& msg)
	{
		Info(*msg);
	}


	Float2 PlatformBase::GetMousePosition()
	{
		/*const Window* win = Engine::MainWindow;
		if (win)
			return win->ClientToScreen(win->GetMousePosition());
		return Float2::Minimum;*/
		return Float2::Zero;
	}

	void PlatformBase::SetMousePosition(const Float2& position)
	{
//		const Window* win = Engine::MainWindow;
//		if (win)
//			win->SetMousePosition(win->ScreenToClient(position));
	}

	Rectangle PlatformBase::GetMonitorBounds(const Float2& screenPos)
	{
//		return Math::Rectangle(Float2::Zero, Platform::GetDesktopSize());
		return Rectangle();
	}

	Rectangle PlatformBase::GetVirtualDesktopBounds()
	{
//		return Rectangle(Float2::Zero, Platform::GetDesktopSize());
		return Rectangle();
	}

	Float2 PlatformBase::GetVirtualDesktopSize()
	{
//		return Platform::GetVirtualDesktopBounds().Size;
		return Float2();
	}

	void PlatformBase::GetEnvironmentVariables(Dictionary <String, String>& result)
	{
		// Not supported
	}

	bool PlatformBase::GetEnvironmentVariable(const String& name, String& value)
	{
		// Not supported
		return true;
	}

	bool PlatformBase::SetEnvironmentVariable(const String& name, const String& value)
	{
		// Not supported
		return true;
	}

	int32 PlatformBase::CreateProcess(CreateProcessSettings& settings)
	{
		// Not supported
		return -1;
	}

//	PRAGMA_DISABLE_DEPRECATION_WARNINGS



	int32 PlatformBase::StartProcess(const StringView& filename, const StringView& args, const StringView& workingDir, bool hiddenWindow, bool waitForEnd)
	{
		CreateProcessSettings procSettings;
		procSettings.FileName = filename;
		procSettings.Arguments = args;
		procSettings.WorkingDirectory = workingDir;
		procSettings.HiddenWindow = hiddenWindow;
		procSettings.WaitForEnd = waitForEnd;
		procSettings.LogOutput = waitForEnd;
		procSettings.ShellExecute = true;
		return Platform::CreateProcess(procSettings);
	}

	int32 PlatformBase::RunProcess(const StringView& cmdLine, const StringView& workingDir, bool hiddenWindow)
	{
		CreateProcessSettings procSettings;
		procSettings.FileName = cmdLine;
		procSettings.WorkingDirectory = workingDir;
		procSettings.HiddenWindow = hiddenWindow;
		return Platform::CreateProcess(procSettings);
	}

	int32 PlatformBase::RunProcess(const StringView& cmdLine, const StringView& workingDir, const Dictionary <String, String>& environment, bool hiddenWindow)
	{
		CreateProcessSettings procSettings;
		procSettings.FileName = cmdLine;
		procSettings.WorkingDirectory = workingDir;
		procSettings.Environment = environment;
		procSettings.HiddenWindow = hiddenWindow;
		return Platform::CreateProcess(procSettings);
	}

//	PRAGMA_ENABLE_DEPRECATION_WARNINGS

	List<PlatformBase::StackFrame> PlatformBase::GetStackFrames(int32 skipCount, int32 maxDepth, void* context)
	{
		return List<PlatformBase::StackFrame>();
	}

	String PlatformBase::GetStackTrace(int32 skipCount, int32 maxDepth, void* context)
	{
		StringBuilder result;
		List <StackFrame> stackFrames = Platform::GetStackFrames(skipCount, maxDepth, context);
		for (const auto& frame : stackFrames)
		{
			StringAsUTF16 < ARRAY_SIZE(StackFrame::FunctionName) > functionName(frame.FunctionName);
			const StringView functionNameStr(functionName.Get());
			if (StringUtils::Length(frame.FileName) != 0)
			{
				StringAsUTF16 < ARRAY_SIZE(StackFrame::FileName) > fileName(frame.FileName);
				result.Append(SE_TEXT("   in ")).Append(functionNameStr);
				if (!functionNameStr.EndsWith(')'))
					result.Append(SE_TEXT("()"));
				result.AppendFormat(SE_TEXT("at {0}:{1}\n"), fileName.Get(), frame.LineNumber);
			}
			else if (StringUtils::Length(frame.FunctionName) != 0)
			{
				result.Append(SE_TEXT("   at ")).Append(functionNameStr);
				if (!functionNameStr.EndsWith(')'))
					result.Append(SE_TEXT("()"));
				result.Append('\n');
			}
			else
			{
				result.AppendFormat(SE_TEXT("   at 0x{0:x}\n"), (uint64)frame.ProgramCounter);
			}
		}
		return result.ToString();
	}

	void PlatformBase::CollectCrashData(const String& crashDataFolder, void* context)
	{
	}

	const Char* ToString(PlatformType type)
	{
		switch (type)
		{
		case PlatformType::Windows:
			return SE_TEXT("Windows");
		case PlatformType::XboxOne:
			return SE_TEXT("Xbox One");
		case PlatformType::UWP:
			return SE_TEXT("Windows Store");
		case PlatformType::Linux:
			return SE_TEXT("Linux");
		case PlatformType::PS4:
			return SE_TEXT("PlayStation 4");
		case PlatformType::XboxScarlett:
			return SE_TEXT("Xbox Scarlett");
		case PlatformType::Android:
			return SE_TEXT("Android");
		case PlatformType::Switch:
			return SE_TEXT("Switch");
		case PlatformType::PS5:
			return SE_TEXT("PlayStation 5");
		case PlatformType::Mac:
			return SE_TEXT("Mac");
		case PlatformType::iOS:
			return SE_TEXT("iOS");
		default:
			return SE_TEXT("");
		}
	}

	const Char* ToString(ArchitectureType type)
	{
		switch (type)
		{
		case ArchitectureType::AnyCPU:
			return SE_TEXT("AnyCPU");
		case ArchitectureType::x86:
			return SE_TEXT("x86");
		case ArchitectureType::x64:
			return SE_TEXT("x64");
		case ArchitectureType::ARM:
			return SE_TEXT("ARM");
		case ArchitectureType::ARM64:
			return SE_TEXT("ARM64");
		default:
			return SE_TEXT("");
		}
	}

}