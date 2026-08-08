
add_subdirectory(Binary)
add_subdirectory(Internal)
add_subdirectory(ManagedCLR)


RUNTIME_ADD(
        Events.h
        Scripting.h
        Scripting.cpp
        ScriptingType.h
        ScriptingType.cpp
        ScriptingTypeDefine.h
        ScriptingObject.h
        ScriptingObject.cpp

#        CS
        Scripting.cs
        ScriptingLoadContext.cs
)

