
#add_subdirectory(Platform)
add_subdirectory(Fonts)
add_subdirectory(ToolsUI)
add_subdirectory(Widgets)

RUNTIME_ADD(
        GUIFont.h
        GUIFont.cpp

        GUIStyle.h
        GUIStyle.cpp
        GUILayout.h
        GUILayout.cpp
        GUIGizmos.h
        GUIGizmos.cpp
)
