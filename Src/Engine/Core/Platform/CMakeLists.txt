
add_subdirectory(Base)
add_subdirectory(Win32)
add_subdirectory(Windows)

CORE_ADD(
        Defines.h
        StringUtils.h
        CriticalSection.h
        ConditionVariable.h
        FileSystem.h
        FileSystemWatcher.h
        Platform.h
        Clipboard.h
        MessageBox.h

        Window.h
        CreateProcessSettings.h
        CreateWindowSettings.h

        CPUInfo.h
        Thread.h
        File.h
        User.h
        Types.h
        MemoryStats.h
        IGuiData.h

        WindowsManager.h
)