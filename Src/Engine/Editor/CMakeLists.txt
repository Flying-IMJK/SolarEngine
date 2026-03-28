# 设置工程名 使用语言c/c++
set(EDITOR_TARGET SE.Editor)

include(${EngineLibDir}/ISPCTextureCompressor/link.cmake)

macro(EDITOR_ADD)
    set(SOURCE_FILES "")
    set(HEAD_FILES "")
    foreach(SOURCE_FILE ${ARGN})
        if(SOURCE_FILE MATCHES ".*\.h")
            if(SOURCE_FILE MATCHES "^/.*")
                list(APPEND HEAD_FILES ${SOURCE_FILE})
            else()
                list(APPEND HEAD_FILES ${CMAKE_CURRENT_SOURCE_DIR}/${SOURCE_FILE})
            endif()
        else()
            if(SOURCE_FILE MATCHES "^/.*")
                list(APPEND SOURCE_FILES ${SOURCE_FILE})
            else()
                list(APPEND SOURCE_FILES ${CMAKE_CURRENT_SOURCE_DIR}/${SOURCE_FILE})
            endif()
        endif()
    endforeach()
    set(EditorHeadFiles ${EditorHeadFiles} ${HEAD_FILES} PARENT_SCOPE)
    set(EditorSourceFiles ${EditorSourceFiles} ${SOURCE_FILES} PARENT_SCOPE)
endmacro()


set(EditorHeadFiles
    ${CMAKE_CURRENT_SOURCE_DIR}/API.h
    ${CMAKE_CURRENT_SOURCE_DIR}/EditorApp.h
    ${CMAKE_CURRENT_SOURCE_DIR}/EditorContext.h
    ${CMAKE_CURRENT_SOURCE_DIR}/EditorAssets.h
    ${CMAKE_CURRENT_SOURCE_DIR}/EditorIcons.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Test.h
)

set(EditorSourceFiles
    ${CMAKE_CURRENT_SOURCE_DIR}/EditorApp.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/EditorContext.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/EditorAssets.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Test.cpp
)

add_subdirectory(_Generated)
add_subdirectory(ThirdParty)
add_subdirectory(Core)
add_subdirectory(Modules)
add_subdirectory(GUI)
add_subdirectory(Viewport)
add_subdirectory(CustomEditors)
add_subdirectory(Resource)
add_subdirectory(Windows)
add_subdirectory(SceneGraph)


source_group(TREE ${EngineEditorDir} FILES ${EditorSourceFiles} ${EditorHeadFiles})

# 引擎运行时库
add_library(${EDITOR_TARGET} SHARED "${EditorHeadFiles}" "${EditorSourceFiles}")

target_include_directories(${EDITOR_TARGET}
    PUBLIC ${EngineDir}
    PUBLIC ${EngineCoreDir}/ThirdParty
    PUBLIC ${EngineEditorDir}/ThirdParty
    PUBLIC ${EngineRuntimeDir}/ThirdParty
)

target_link_libraries(${EDITOR_TARGET} PUBLIC SE.Core SE.Runtime)

add_dependencies(${EDITOR_TARGET} SE.Runtime)

# 第三方链接库
ISPCTextureCompressorLink(${EDITOR_TARGET})

target_compile_definitions(${EDITOR_TARGET} PRIVATE SE_DLL_EDITOR)

### precompile
PRECOMPILE_PROJECT(${EDITOR_TARGET} SE.Editor ${EditorHeadFiles})
