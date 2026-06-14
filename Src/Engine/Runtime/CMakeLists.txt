# 设置工程名 使用语言c/c++
set(RUNTIME_TARGET SERuntime)

include(${EngineLibDir}/ISPCTextureCompressor/link.cmake)
include(${EngineLibDir}/ShaderCompiler/DirectXShaderCompiler/DCCompiler.cmake)
include(${EngineLibDir}/GameNetworkingSockets/link.cmake)
include(${EngineLibDir}/slang/link.cmake)
include(${EngineLibDir}/NetHost/link.cmake)

macro(RUNTIME_ADD)
    set(SOURCE_FILES "")
    set(HEAD_FILES "")
    set(CSHARP_FILES "")
    foreach(SOURCE_FILE ${ARGN})
        if(SOURCE_FILE MATCHES ".*\.h")
            if(SOURCE_FILE MATCHES "^/.*")
                list(APPEND HEAD_FILES ${SOURCE_FILE})
            else()
                list(APPEND HEAD_FILES ${CMAKE_CURRENT_SOURCE_DIR}/${SOURCE_FILE})
            endif()
        elseif(SOURCE_FILE MATCHES ".*\.cs")
            if(SOURCE_FILE MATCHES "^/.*")
                list(APPEND CSHARP_FILES ${SOURCE_FILE})
            else()
                list(APPEND CSHARP_FILES ${CMAKE_CURRENT_SOURCE_DIR}/${SOURCE_FILE})
            endif()
        else()
            if(SOURCE_FILE MATCHES "^/.*")
                list(APPEND SOURCE_FILES ${SOURCE_FILE})
            else()
                list(APPEND SOURCE_FILES ${CMAKE_CURRENT_SOURCE_DIR}/${SOURCE_FILE})
            endif()
        endif()
    endforeach()
    set(RuntimeHeadFiles ${RuntimeHeadFiles} ${HEAD_FILES} PARENT_SCOPE)
    set(RuntimeSourceFiles ${RuntimeSourceFiles} ${SOURCE_FILES} PARENT_SCOPE)
    set(RuntimeCSharpFiles ${RuntimeCSharpFiles} ${CSHARP_FILES} PARENT_SCOPE)
endmacro()


set(RuntimeHeadFiles
        ${CMAKE_CURRENT_SOURCE_DIR}/API.h
        ${CMAKE_CURRENT_SOURCE_DIR}/App.h
        ${CMAKE_CURRENT_SOURCE_DIR}/Engine.h
        ${CMAKE_CURRENT_SOURCE_DIR}/EngineContext.h
#        ${CMAKE_CURRENT_SOURCE_DIR}/ShaderReflection.h
)
set(RuntimeSourceFiles
        ${CMAKE_CURRENT_SOURCE_DIR}/API.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/Engine.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/EngineContext.cpp
#        ${CMAKE_CURRENT_SOURCE_DIR}/ShaderReflection.cpp
)
set(RuntimeCSharpFiles
)


add_subdirectory(ThirdParty)
add_subdirectory(CSharp)
add_subdirectory(Network)
add_subdirectory(System)
add_subdirectory(Project)
add_subdirectory(SGUI)
add_subdirectory(UI)
add_subdirectory(Settings)
add_subdirectory(Level)
add_subdirectory(Scripting)
add_subdirectory(Graph)
add_subdirectory(Graphics)
add_subdirectory(GraphicsDevice)
add_subdirectory(ShaderCompilation)
add_subdirectory(Render)
add_subdirectory(Resource)
add_subdirectory(Utilities)
add_subdirectory(_Generated)


source_group(TREE ${EngineRuntimeDir} FILES ${RuntimeSourceFiles} ${RuntimeHeadFiles})

# 引擎C++运行时库
add_library(${RUNTIME_TARGET} SHARED "${RuntimeHeadFiles}" "${RuntimeSourceFiles}")

# 引擎C#运行时库
CSHARP_TARGET(${RUNTIME_TARGET} "${RuntimeCSharpFiles}" "${CMAKE_CURRENT_SOURCE_DIR}/RuntimeCSharp.csproj")

target_include_directories(${RUNTIME_TARGET}
    PUBLIC ${EngineDir}
    PUBLIC ${EngineRuntimeDir}/ThirdParty
    PUBLIC ${EngineLibDir}/assimp/include
)


target_link_libraries(${RUNTIME_TARGET} PUBLIC SECore assimp ShaderCompiler)
if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    target_link_libraries(${RUNTIME_TARGET} PUBLIC delayimp )
endif()
# 运行时库 第三方链接库
ADD_DELAYLOAD_FLAGS(${RUNTIME_TARGET} ShaderCompiler)
#assimp-vc143-mt
# 第三方链接库
DirectXShaderCompilerCopy(${RUNTIME_TARGET})
GameNetworkingSocketsLink(${RUNTIME_TARGET})
ISPCTextureCompressorLink(${RUNTIME_TARGET})
SlangLink(${RUNTIME_TARGET})
NetCoreLink(${RUNTIME_TARGET})

add_dependencies(${RUNTIME_TARGET} ${RUNTIME_TARGET}.CSharp SECore)

target_compile_definitions(${RUNTIME_TARGET} PRIVATE "SE_DLL_RUNTIME" "COMPILE_USE_STB")

### precompile
PRECOMPILE_PROJECT(${RUNTIME_TARGET} SERuntime ${RuntimeHeadFiles})
