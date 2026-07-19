#ifdef PLATFORM_WIN32
#include "Runtime/Core/Platform/Platform.h"
#include "Runtime/Core/Logging/LoggingSystem.h"
#include "Runtime/Core/Types/Collections/List.h"
#include "Runtime/Core/Memory/Memory.h"
#include "Runtime/Core/Types/Strings/String.h"

#ifndef NOMINMAX
#define NOMINMAX
#endif

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>
#include <dbghelp.h>
#include <shellapi.h>
#include <shlobj.h>
#include <strsafe.h>
#include <Psapi.h>
#include <stdio.h>
#include <tlhelp32.h>

#pragma comment(lib, "Dbghelp.lib")

//-------------------------------------------------------------------------

namespace SE::Platform
{

    struct WinModule : public MODULE
    {
        HMODULE hmodule;
    };

    struct WinFunProc : public FunProc
    {
        FARPROC proc;
    };

    //-------------------------------------------------------------------------
    // Stack Walking
    //-------------------------------------------------------------------------

    struct Symbol
    {
        uint64 m_address;
        uint64 m_baseOfImage;
        String m_imageName;
        String m_file;
        String m_symbol;
        uint32 m_lineNumber;
    };

    List<Symbol> WalkStack(PCONTEXT pExceptionContext)
    {
		List<Symbol> callstack;

        HANDLE process = GetCurrentProcess();

        // Initialize the symbol handler
        //-------------------------------------------------------------------------

        SymSetOptions(SYMOPT_DEFERRED_LOADS | SYMOPT_LOAD_LINES | SYMOPT_UNDNAME);
        if (!SymInitializeW(process, nullptr, true))
        {
            ENGINE_TRACE_MSG(GetLastErrorMessage().Get());
            return callstack;
        }

        // Walk the stack
        //-------------------------------------------------------------------------

        // Copy the context because StackWalk64 modifies it
        CONTEXT context = *pExceptionContext;
        context.ContextFlags = CONTEXT_FULL;

        // Grab the exception callstack
        STACKFRAME64 stackFrame = {};
        stackFrame.AddrPC.Mode = AddrModeFlat;
        stackFrame.AddrFrame.Mode = AddrModeFlat;
        stackFrame.AddrStack.Mode = AddrModeFlat;
        stackFrame.AddrPC.Offset = context.Rip;
        stackFrame.AddrFrame.Offset = context.Rsp;
        stackFrame.AddrStack.Offset = context.Rsp;

        // Process each frame of the stack
        List<uint64, InlinedAllocation<256>> callstackAddresses;
        while (StackWalk64(IMAGE_FILE_MACHINE_AMD64, process, GetCurrentThread(),
			&stackFrame, &context, nullptr,
			&SymFunctionTableAccess64, &SymGetModuleBase64, nullptr))
        {
            uint64 const address = stackFrame.AddrPC.Offset;
            callstackAddresses.Add(address);
        }

        // Symbolificate the stack addresses
        //-------------------------------------------------------------------------

        callstack.Resize(callstackAddresses.Count());
        for (uint64 const address : callstackAddresses)
        {
            // retrieve module information
            IMAGEHLP_MODULE64 module = {};
            module.SizeOfStruct = sizeof(IMAGEHLP_MODULE64);
            BOOL const successfulModule = SymGetModuleInfo64(process, address, &module);

            // retrieve filename and line number
            DWORD displacement = 0u;
            IMAGEHLP_LINE64 line = {};
            line.SizeOfStruct = sizeof(IMAGEHLP_LINE64);
            BOOL const successfulLine = SymGetLineFromAddr64(process, address, &displacement, &line);

            // retrieve function
            DWORD64 displacement64 = 0u;
            ULONG64 buffer[(sizeof(SYMBOL_INFO) + MAX_SYM_NAME * sizeof(char) + sizeof(ULONG64) - 1u) / sizeof(ULONG64)] = {};
            SYMBOL_INFO *symbolInfo = reinterpret_cast<SYMBOL_INFO *>(buffer);
            symbolInfo->SizeOfStruct = sizeof(SYMBOL_INFO);
            symbolInfo->MaxNameLen = MAX_SYM_NAME;
            BOOL const successfulSymbol = SymFromAddr(process, address, &displacement64, symbolInfo);

            callstack.Add(Symbol{
                address,
                successfulModule ? module.BaseOfImage : 0ull,
				successfulModule ? module.ImageName : String(),
				successfulLine ? line.FileName : String(),
				successfulSymbol ? symbolInfo->Name : String(),
                successfulLine ? line.LineNumber : 0u});
        }

        if (!SymCleanup(process))
        {
            ENGINE_TRACE_MSG(GetLastErrorMessage().Get());
        }

        //-------------------------------------------------------------------------

        return callstack;
    }

    //-------------------------------------------------------------------------
    // Crash Handling
    //-------------------------------------------------------------------------

    void GenerateCrashDump(EXCEPTION_POINTERS *pExceptionPointers)
    {
        // Get process path
        //-------------------------------------------------------------------------
        #ifdef UNICODE
        WCHAR processPath[MAX_PATH];
        HANDLE pProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, false, GetCurrentProcessId());
        if (pProcess != nullptr)
        {
            GetModuleFileNameEx(pProcess, 0, processPath, MAX_PATH);
            CloseHandle(pProcess);
        }

        // Get directory
        for (int i = (int)wcslen(processPath) - 1; i > 0; --i)
        {
            if (processPath[i] == '\\')
            {
                processPath[i] = '\0';
                break;
            }
        }

        #else

        CHAR processPath[MAX_PATH];
        HANDLE pProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, false, GetCurrentProcessId());
        if (pProcess != nullptr)
        {
            GetModuleFileNameEx(pProcess, 0, processPath, MAX_PATH);
            CloseHandle(pProcess);
        }

        // Get directory
        for (int i = (int)strlen(processPath) - 1; i > 0; --i)
        {
            if (processPath[i] == '\\')
            {
                processPath[i] = '\0';
                break;
            }
        }
        #endif




        // Create file name for dump
        //-------------------------------------------------------------------------

        SYSTEMTIME stLocalTime;
        GetLocalTime(&stLocalTime);

        #ifdef UNICODE

        WCHAR crashDumpDirectoryPath[MAX_PATH];
        StringCchPrintf(crashDumpDirectoryPath, MAX_PATH, L"%s\\%s", processPath, "CrashDumps");
        CreateDirectory(crashDumpDirectoryPath, NULL);

        WCHAR minidumpFilePath[MAX_PATH];
        StringCchPrintf(minidumpFilePath, MAX_PATH, L"%s\\%04d%02d%02d-%02d%02d%02d-%ld-%ld.dmp",
                        crashDumpDirectoryPath,
                        stLocalTime.wYear, stLocalTime.wMonth, stLocalTime.wDay,
                        stLocalTime.wHour, stLocalTime.wMinute, stLocalTime.wSecond,
                        GetCurrentProcessId(), GetCurrentThreadId());

        WCHAR stackTraceFilePath[MAX_PATH];
        StringCchPrintf(stackTraceFilePath, MAX_PATH, L"%s\\%04d%02d%02d-%02d%02d%02d-%ld-%ld.txt",
                        crashDumpDirectoryPath,
                        stLocalTime.wYear, stLocalTime.wMonth, stLocalTime.wDay,
                        stLocalTime.wHour, stLocalTime.wMinute, stLocalTime.wSecond,
                        GetCurrentProcessId(), GetCurrentThreadId());

        #else

        CHAR crashDumpDirectoryPath[MAX_PATH];
        StringCchPrintf(crashDumpDirectoryPath, MAX_PATH, "%s\\%s", processPath, "CrashDumps");
        CreateDirectory(crashDumpDirectoryPath, NULL);

        CHAR minidumpFilePath[MAX_PATH];
        StringCchPrintf(minidumpFilePath, MAX_PATH, "%s\\%04d%02d%02d-%02d%02d%02d-%ld-%ld.dmp",
                        crashDumpDirectoryPath,
                        stLocalTime.wYear, stLocalTime.wMonth, stLocalTime.wDay,
                        stLocalTime.wHour, stLocalTime.wMinute, stLocalTime.wSecond,
                        GetCurrentProcessId(), GetCurrentThreadId());

        CHAR stackTraceFilePath[MAX_PATH];
        StringCchPrintf(stackTraceFilePath, MAX_PATH, "%s\\%04d%02d%02d-%02d%02d%02d-%ld-%ld.txt",
                        crashDumpDirectoryPath,
                        stLocalTime.wYear, stLocalTime.wMonth, stLocalTime.wDay,
                        stLocalTime.wHour, stLocalTime.wMinute, stLocalTime.wSecond,
                        GetCurrentProcessId(), GetCurrentThreadId());

        #endif

        // Create dump
        //-------------------------------------------------------------------------

        HANDLE hDumpFile = CreateFile(minidumpFilePath, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_WRITE | FILE_SHARE_READ, 0, CREATE_ALWAYS, 0, 0);

        MINIDUMP_EXCEPTION_INFORMATION minidumpExceptionInfo;
        minidumpExceptionInfo.ThreadId = GetCurrentThreadId();
        minidumpExceptionInfo.ExceptionPointers = pExceptionPointers;
        minidumpExceptionInfo.ClientPointers = TRUE;

        MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), hDumpFile, MiniDumpWithDataSegs, (pExceptionPointers == nullptr) ? NULL : &minidumpExceptionInfo, NULL, NULL);

        CloseHandle(hDumpFile);

        // Generate stack
        //-------------------------------------------------------------------------

        auto stack = WalkStack(pExceptionPointers->ContextRecord);
        if (!stack.IsEmpty())
        {
            String stackFileData;

            for (auto const &stackEntry : stack)
            {
                stackFileData.Append(String::Format("{0} ({1}:{2})\r\n", stackEntry.m_symbol, stackEntry.m_file, stackEntry.m_lineNumber));
            }

            HANDLE hStackTraceFile = CreateFile(stackTraceFilePath, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_WRITE | FILE_SHARE_READ, 0, CREATE_ALWAYS, 0, 0);

            DWORD dwBytesToWrite = (DWORD)StringUtils::Length(stackFileData.Get());
            DWORD dwBytesWritten = 0;
            WriteFile(hStackTraceFile, stackFileData.Get(), dwBytesToWrite, &dwBytesWritten, NULL);

            CloseHandle(hStackTraceFile);
        }
    }

    LONG WINAPI VectoredExceptionHandler(_EXCEPTION_POINTERS *pExceptionPtrs)
    {
        switch (pExceptionPtrs->ExceptionRecord->ExceptionCode)
        {
        case EXCEPTION_ACCESS_VIOLATION:
        case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:
        case EXCEPTION_DATATYPE_MISALIGNMENT:
        case EXCEPTION_FLT_STACK_CHECK:
        case EXCEPTION_ILLEGAL_INSTRUCTION:
        case EXCEPTION_NONCONTINUABLE_EXCEPTION:
        case EXCEPTION_PRIV_INSTRUCTION:
        case EXCEPTION_STACK_OVERFLOW:
        case EXCEPTION_BREAKPOINT:
        case EXCEPTION_SINGLE_STEP:
        {
            GenerateCrashDump(pExceptionPtrs);
            Log::System::SaveToFile();
        }
        break;
        }

        return EXCEPTION_CONTINUE_SEARCH;
    }

    static PVOID g_pExceptionHandler = nullptr;

    //-------------------------------------------------------------------------
    // Platform
    //-------------------------------------------------------------------------

    void Initialize()
    {
        g_pExceptionHandler = AddVectoredExceptionHandler(1, VectoredExceptionHandler);
    }

    void Shutdown()
    {
        RemoveVectoredExceptionHandler(g_pExceptionHandler);
    }

    bool EngineLoadLibrary(const char* lib, MODULE* & module)
    {
        HMODULE hmodule;
#ifdef UNICODE
        String wPath;
        StringTool::StringToWString(lib, wPath);
        hmodule = LoadLibrary(wPath.c_str());
#else
        hmodule = LoadLibrary(lib);
#endif
        if(!hmodule)
        {
            module = nullptr;
            return false;
        }

        WinModule* winModule = New<WinModule>();
        winModule->hmodule = hmodule;

        module = winModule;
        return true;
    }

    void EngineFreeLibrary(MODULE* lib)
    {
        ENGINE_ASSERT(lib);

        WinModule* winModule = static_cast<WinModule*>(lib);
        ENGINE_ASSERT(winModule->hmodule);

        FreeLibrary(winModule->hmodule);
        Delete(lib);
    }

    void* EngineGetProcAddress(MODULE* module, const char* name)
    {
        WinModule* winModule = static_cast<WinModule*>(module);

        return (void*)GetProcAddress(winModule->hmodule, name);
    }



    uint32 GetProcessID(char const *processName)
    {
        uint32 processID = 0;
        PROCESSENTRY32 entry;
        entry.dwSize = sizeof(PROCESSENTRY32);

        HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);

        if (Process32First(snapshot, &entry))
        {   
            char* szExeFilePtr;

            #ifdef UNICODE
            char szExeFile[MAX_PATH];
            #endif

            while (Process32Next(snapshot, &entry))
            {
                #ifdef UNICODE
                StringTool::WCharToChar(entry.szExeFile, szExeFile);
                szExeFilePtr = szExeFile;
                #else
                szExeFilePtr = entry.szExeFile;
                #endif

                if (strcmp(szExeFilePtr, processName) == 0)
                {
                    processID = entry.th32ProcessID;
                }
            }
        }

        CloseHandle(snapshot);
        return processID;
    }

    uint32 StartProcess(char const *exePath, char const *cmdLine)
    {
        PROCESS_INFORMATION processInfo;
        memset(&processInfo, 0, sizeof(processInfo));

        STARTUPINFO startupInfo;
        memset(&startupInfo, 0, sizeof(startupInfo));
        startupInfo.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
        startupInfo.hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);
        startupInfo.hStdError = GetStdHandle(STD_ERROR_HANDLE);
        startupInfo.dwFlags = STARTF_USESTDHANDLES;
        startupInfo.cb = sizeof(startupInfo);


        bool state = false;

        #ifdef UNICODE
        wchar_t fullCmdLine[MAX_PATH];
        if (cmdLine != nullptr)
        {
            //Printf(fullCmdLine, MAX_PATH, "%s %s", exePath, cmdLine);
            StringTool::CharToWChar(fmt::format("{0} {1}", exePath, cmdLine).c_str(), fullCmdLine);
        }
        else
        {
            //Printf(fullCmdLine, MAX_PATH, "%s", exePath);
            StringTool::CharToWChar(fmt::format("{0}", exePath).c_str(), fullCmdLine);
        }
        state = CreateProcess(nullptr, fullCmdLine, nullptr, nullptr, false, 0, nullptr, nullptr, &startupInfo, &processInfo);
        #else
        String fullCmdLine;
        if (cmdLine != nullptr)
        {
            //Printf(fullCmdLine, MAX_PATH, "%s %s", exePath, cmdLine);
            fullCmdLine = String::Format("{0} {1}", exePath, cmdLine);
        }
        else
        {
            //Printf(fullCmdLine, MAX_PATH, "%s", exePath);
            fullCmdLine = String::Format("{0}", exePath);
        }
        state = CreateProcess(nullptr, fullCmdLine.Get(), nullptr, nullptr, false, 0, nullptr, nullptr, &startupInfo, &processInfo);
        #endif

        if (state)
        {
            return processInfo.dwProcessId;
        }

        return 0;
    }

    bool KillProcess(uint32 processID)
    {
        ENGINE_ASSERT(processID != 0);
        HANDLE pProcess = OpenProcess(PROCESS_TERMINATE, false, processID);
        if (pProcess != nullptr)
        {
            return TerminateProcess(pProcess, 0) > 0;
        }

        return false;
    }

    String GetProcessPath(uint32 processID)
    {
        ENGINE_ASSERT(processID != 0);

        String returnPath;
        HANDLE pProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, false, processID);
        if (pProcess != nullptr)
        {
            TCHAR currentProcessPath[MAX_PATH + 1];
            if (GetModuleFileNameEx(pProcess, 0, currentProcessPath, MAX_PATH + 1))
            {
                #ifdef UNICODE
                StringTool::WStringToString(currentProcessPath, returnPath);
                #else
                returnPath = currentProcessPath;
                #endif
            }
            CloseHandle(pProcess);
        }

        return returnPath;
    }

    String GetCurrentModulePath()
    {
        String path;
        TCHAR exepath[MAX_PATH + 1];
        if (GetModuleFileName(0, exepath, MAX_PATH + 1))
        {
            #ifdef UNICODE
            char exepath_Temp[MAX_PATH + 1];
            StringTool::WCharToChar(exepath, exepath_Temp, MAX_PATH + 1);
            path = exepath_Temp;
            #else
            path = exepath;
            #endif
        }

        return path;
    }

    String GetLastErrorMessage()
    {
        auto const errorID = GetLastError();

        LPSTR pMessageBuffer = nullptr;
        auto size = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, errorID, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&pMessageBuffer, 0, NULL);
        String message(pMessageBuffer, size);
        LocalFree(pMessageBuffer);
        return message;
    }

    String GetShortPath(String const &origPath)
    {
        #ifdef UNICODE
        TCHAR pathW[MAX_PATH + 1];
        String path;

        String origpathW;
        StringTool::StringToWString(origPath.c_str(), origpathW);
        DWORD const returnValue = GetShortPathName(origpathW.c_str(), pathW, MAX_PATH + 1);

        StringTool::WStringToString(pathW, path);
        #else

        TCHAR path[MAX_PATH + 1];
        DWORD const returnValue = GetShortPathName(origPath.Get(), path, MAX_PATH + 1);

        #endif
        
        if (returnValue > 0 && returnValue <= MAX_PATH)
        {
            return String(path);
        }

        return String();
    }

    String GetLongPath(String const &origPath)
    {
        char path[MAX_PATH + 1];
        DWORD const returnValue = GetLongPathNameA(origPath.Get(), path, MAX_PATH + 1);

        if (returnValue > 0 && returnValue <= MAX_PATH)
        {
            return String(path);
        }

        return String();
    }

    void OpenInExplorer(char const *path)
    {
        ENGINE_ASSERT(path != nullptr && path[0] != 0);
        #ifdef UNICODE
        String cmdLine;
        StringTool::StringToWString(fmt::format("/select, {0}", path).c_str(), cmdLine);
        ShellExecute(0, NULL, L"explorer.exe", cmdLine.c_str(), NULL, SW_SHOWNORMAL);
        #else
        String cmdLine = String::Format("/select, {0}", path);
        ShellExecute(0, NULL, "explorer.exe", cmdLine.Get(), NULL, SW_SHOWNORMAL);
        #endif
    }
}
#endif