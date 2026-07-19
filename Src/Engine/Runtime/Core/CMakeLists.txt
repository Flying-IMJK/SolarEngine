
add_subdirectory(Logging)
add_subdirectory(Math)
add_subdirectory(Memory)
add_subdirectory(Platform)
add_subdirectory(Profiler)
add_subdirectory(Scripting)
add_subdirectory(Serialization)
add_subdirectory(Thread)
add_subdirectory(Types)
add_subdirectory(TypeSystem)



RUNTIME_ADD(
        Systems.h
        Systems.cpp
        Templates.h

        Encoding.h
        Encoding.cpp
        Formatting.h
        IniFile.h
        IniFile.cpp

        Timers.h
        Time.h
        Time.cpp

        Singleton.hpp
)