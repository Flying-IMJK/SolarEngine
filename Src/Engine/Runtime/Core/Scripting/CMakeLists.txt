
add_subdirectory(Binary)
add_subdirectory(Internal)
add_subdirectory(ManagedCLR)


RUNTIME_ADD(
        Events.h
        Scripting.h
        Scripting.cpp
        ScriptingType.h
        ScriptingType.cpp
        ScriptingObject.h
        ScriptingObject.cpp

#        CS
        ScriptingLoadContext.cs
        ScriptingObject.cs
)

