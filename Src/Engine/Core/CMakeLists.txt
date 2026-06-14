
# 设置工程名 使用语言c/c++
set(CORE_TARGET SECore)

macro(CORE_ADD)
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
    set(CoreHeadFiles ${CoreHeadFiles} ${HEAD_FILES} PARENT_SCOPE)
    set(CoreSourceFiles ${CoreSourceFiles} ${SOURCE_FILES} PARENT_SCOPE)
endmacro()

set(CoreHeadFiles
        ${CMAKE_CURRENT_SOURCE_DIR}/Compiler.h
        ${CMAKE_CURRENT_SOURCE_DIR}/API.h
        ${CMAKE_CURRENT_SOURCE_DIR}/Systems.h
        ${CMAKE_CURRENT_SOURCE_DIR}/Templates.h
        ${CMAKE_CURRENT_SOURCE_DIR}/CoreModule.h
        ${CMAKE_CURRENT_SOURCE_DIR}/CoreModule.cpp
)

set(CoreSourceFiles
        ${CMAKE_CURRENT_SOURCE_DIR}/Systems.cpp
)


add_subdirectory(_Generated)
add_subdirectory(Math)
add_subdirectory(Thread)
add_subdirectory(Platform)
add_subdirectory(Input)
add_subdirectory(Profiler)
add_subdirectory(Logging)
add_subdirectory(Memory)
add_subdirectory(Types)
add_subdirectory(TypeSystem)
add_subdirectory(Serialization)
add_subdirectory(ThirdParty)
add_subdirectory(Utilities)

source_group(TREE ${EngineCoreDir} FILES ${CoreSourceFiles} ${CoreHeadFiles})

# 引擎核心库
add_library(${CORE_TARGET} SHARED ${CoreSourceFiles} ${CoreHeadFiles})

target_include_directories(${CORE_TARGET}
    PUBLIC ${EngineDir}
    PUBLIC ${EngineDir}/Core/ThirdParty
    PUBLIC ${EngineLibDir}/fmt/include
    PUBLIC ${EngineLibDir}/sqlite3
    PUBLIC ${EngineLibDir}/Freetype/include
)

# 引擎核心库 第三方链接库
target_link_libraries(${CORE_TARGET} PUBLIC sqlite3 fmt ws2_32 freetype)

if (WIN32)
    target_link_libraries(${CORE_TARGET} PUBLIC Xinput Dbghelp)
endif()

target_compile_definitions(${CORE_TARGET} PRIVATE SE_DLL_CORE TRACY_ENABLE TRACY_EXPORTS)

### precompile
PRECOMPILE_PROJECT(${CORE_TARGET} ${CORE_TARGET} ${CoreHeadFiles})
