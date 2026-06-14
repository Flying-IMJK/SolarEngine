
set(EDITOR_TARGET_NAME WindEditor)

set(EditorHeadFiles )
set(EditorSourceFiles main.cpp)
set(EditorResourceFiles "Resource.h" "Resource.rc")


source_group(TREE ${EngineRootDir}/Applications/Win32 FILES ${EditorHeadFiles} ${EditorSourceFiles} ${EditorResourceFiles})

add_executable(${EDITOR_TARGET_NAME} WIN32 ${EditorHeadFiles} ${EditorSourceFiles} ${EditorResourceFiles})

target_link_libraries(${EDITOR_TARGET_NAME} PRIVATE SECore SERuntime SEEditor)

set_target_properties(${EDITOR_TARGET_NAME} PROPERTIES OUTPUT_NAME "SE")

target_include_directories(${EDITOR_TARGET_NAME} PRIVATE
    ${EngineLibDir}
    ${EngineDir}
)

add_dependencies(${EDITOR_TARGET_NAME} SECore SERuntime SEEditor)