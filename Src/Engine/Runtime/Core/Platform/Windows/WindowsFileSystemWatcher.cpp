
#if PLATFORM_WINDOWS

#include "WindowsFileSystemWatcher.h"
#include "Runtime/Core/Platform/CriticalSection.h"
#include "Runtime/Core/Thread/Threading.h"
#include "Runtime/Core/Logging/Logging.h"
#include "Runtime/Core/Math/Math.h"
#include "Runtime/Core/Types/Collections/List.h"
#include "../FileSystem.h"
#include "../Win32/IncludeWindowsHeaders.h"

namespace SE
{
	BOOL RefreshWatch(WindowsFileSystemWatcher* watcher);

	namespace FileSystemWatchers
	{
		CriticalSection Locker;
		List<WindowsFileSystemWatcher*, InlinedAllocation<16>> Watchers;
		Win32Thread* Thread = nullptr;
		bool ThreadActive;

		int32 Run()
		{
			while (ThreadActive)
			{
				SleepEx(INFINITE, true);
			}
			return 0;
		}

		static void CALLBACK StopProc(ULONG_PTR arg)
		{
			ThreadActive = false;
		}

		static void CALLBACK AddDirectoryProc(ULONG_PTR arg)
		{
			const auto watcher = (FileSystemWatcher*)arg;
			BOOL s = RefreshWatch(watcher);
			ENGINE_ASSERT(s);
		}
	};

	VOID CALLBACK NotificationCompletion(DWORD dwErrorCode, DWORD dwNumberOfBytesTransfered, LPOVERLAPPED lpOverlapped)
	{
		auto watcher = (FileSystemWatcher*)lpOverlapped->hEvent;
		if (dwErrorCode == ERROR_OPERATION_ABORTED ||
			dwNumberOfBytesTransfered <= 0 ||
			!watcher)
		{
			return;
		}

		// Swap buffers
		watcher->CurrentBuffer = (watcher->CurrentBuffer + 1) % 2;

		// Get the new read issued as fast as possible
		if (!watcher->StopNow)
		{
			RefreshWatch(watcher);
		}

		// Process notifications
		auto notify = (FILE_NOTIFY_INFORMATION*)watcher->Buffer[(watcher->CurrentBuffer + 1) % 2];
		do
		{
			// Convert action type
			auto action = FileSystemAction::Unknown;
			switch (notify->Action)
			{
			case FILE_ACTION_ADDED:
				action = FileSystemAction::Create;
				break;
			case FILE_ACTION_REMOVED:
				action = FileSystemAction::Delete;
				break;
			case FILE_ACTION_MODIFIED:
				action = FileSystemAction::Modify;
				break;
			case FILE_ACTION_RENAMED_OLD_NAME:
				action = FileSystemAction::Rename;
				break;
			default:
				action = FileSystemAction::Unknown;
				break;
			}

			if (action == FileSystemAction::Rename)
			{
				FileWatcherEvent event;
				event.action = action;
				// Build path
				String oldPath(notify->FileName, notify->FileNameLength / sizeof(WCHAR));
				event.oldPath = watcher->Directory / oldPath;

				ENGINE_ASSERT( notify->NextEntryOffset != 0 );
				notify = (FILE_NOTIFY_INFORMATION*)((byte*)notify + notify->NextEntryOffset);
				ENGINE_ASSERT(notify->Action == FILE_ACTION_RENAMED_NEW_NAME );

				String newPath(notify->FileName, notify->FileNameLength / sizeof(WCHAR));
				event.path = watcher->Directory / newPath;

				// Send event
				watcher->OnEvent(event);
			}

			if (action == FileSystemAction::Create || action == FileSystemAction::Delete || action == FileSystemAction::Modify)
			{
				// Build path
				String path(notify->FileName, notify->FileNameLength / sizeof(WCHAR));

				FileWatcherEvent event;
				event.path = watcher->Directory / path;
				event.action = action;

				// Send event
				watcher->OnEvent(event);
			}

			// Move to the next notify
			notify = (FILE_NOTIFY_INFORMATION*)((byte*)notify + notify->NextEntryOffset);
		} while (notify->NextEntryOffset != 0);
	}

	// Refreshes the directory monitoring
	BOOL RefreshWatch(WindowsFileSystemWatcher* watcher)
	{
		DWORD dwBytesReturned = 0;
		return ReadDirectoryChangesW(
			watcher->DirectoryHandle,
			watcher->Buffer[watcher->CurrentBuffer],
			FileSystemWatcher::BufferSize,
			watcher->WithSubDirs ? TRUE : FALSE,
			FILE_NOTIFY_CHANGE_CREATION | FILE_NOTIFY_CHANGE_SIZE | FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME,
			&dwBytesReturned,
			(OVERLAPPED*)&watcher->Overlapped,
			NotificationCompletion
		);
	}

	WindowsFileSystemWatcher::WindowsFileSystemWatcher(const String& directory, bool withSubDirs)
		: FileSystemWatcherBase(directory, withSubDirs)
		, StopNow(false)
		, CurrentBuffer(0)
	{
		// Setup
		Platform::MemoryClear(&Overlapped, sizeof(Overlapped));
		((OVERLAPPED&)Overlapped).hEvent = this;

		// Create directory handle for events handling
		DirectoryHandle = CreateFileW(
			*directory,
			FILE_LIST_DIRECTORY,
			FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
			nullptr,
			OPEN_EXISTING,
			FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED,
			nullptr
		);
		if (DirectoryHandle == INVALID_HANDLE_VALUE)
		{
			LOG_WIN32_LAST_ERROR;
			return;
		}

		// Register
		FileSystemWatchers::Locker.Lock();
		FileSystemWatchers::Watchers.Add(this);
		if (!FileSystemWatchers::Thread)
		{
			FileSystemWatchers::ThreadActive = true;
			FileSystemWatchers::Thread = Threading::StartThread(FileSystemWatchers::Run, TEXT("File System Watchers"), ThreadPriority::BelowNormal);
		}
		FileSystemWatchers::Locker.Unlock();

		// Issue the first read
		DWORD state = QueueUserAPC(FileSystemWatchers::AddDirectoryProc, FileSystemWatchers::Thread->GetHandle(), (ULONG_PTR)this);

		ENGINE_ASSERT(state != 0);
	}

	WindowsFileSystemWatcher::~WindowsFileSystemWatcher()
	{
		FileSystemWatchers::Locker.Lock();
		FileSystemWatchers::Watchers.Remove(this);
		FileSystemWatchers::Locker.Unlock();

		if (DirectoryHandle != INVALID_HANDLE_VALUE)
		{
			StopNow = true;

#if WINVER >= 0x600
			CancelIoEx(DirectoryHandle, (OVERLAPPED*)&Overlapped);
#else
			CancelIo(DirectoryHandle);
#endif

			const HANDLE handle = DirectoryHandle;
			DirectoryHandle = INVALID_HANDLE_VALUE;
			WaitForSingleObjectEx(handle, 0, true);

			CloseHandle(handle);
		}

		FileSystemWatchers::Locker.Lock();
		if (FileSystemWatchers::Watchers.IsEmpty() && FileSystemWatchers::Thread)
		{
			FileSystemWatchers::ThreadActive = false;
			QueueUserAPC(FileSystemWatchers::StopProc, FileSystemWatchers::Thread->GetHandle(), 0);
			FileSystemWatchers::Thread->Join();
			Delete(FileSystemWatchers::Thread);
			FileSystemWatchers::Thread = nullptr;
		}
		FileSystemWatchers::Locker.Unlock();
	}
}

#endif
