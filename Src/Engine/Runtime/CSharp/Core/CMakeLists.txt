
add_subdirectory(Log)

RUNTIME_ADD(
        ScriptingLoadContext.cs
        ScriptingObject.cs
        
        NativeInterop.cs
        NativeInterop.Invoker.cs
        NativeInterop.Managed.cs
        NativeInterop.Marshallers.cs
        NativeInterop.Unmanaged.cs
        NativeInterop.Utils.cs
        
        Object.cs
)
