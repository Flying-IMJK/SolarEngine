

if (WIN32)

    if (CMAKE_BUILD_TYPE STREQUAL Debug)
        set(SLANG_LIBRARY_DIR  ${EngineLibDir}/slang/lib)
        set(SLANG_SHARED_LIBRARY_DIR  ${EngineLibDir}/slang/bin)
    else()
        set(SLANG_LIBRARY_DIR  ${EngineLibDir}/slang/lib)
        set(SLANG_SHARED_LIBRARY_DIR  ${EngineLibDir}/slang/bin)
    endif()

elseif(${CMAKE_HOST_SYSTEM_NAME} STREQUAL "Linux")
    set(SLANG_LIBRARY_DIR  ${EngineLibDir}/slang/lib)
    set(SLANG_SHARED_LIBRARY_DIR  ${EngineLibDir}/slang/bin)
    set(CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS} -O3")


elseif(${CMAKE_HOST_SYSTEM_PROCESSOR} STREQUAL "arm64") 
    # link to XCode Toolchains' universal binary libclang.dylib 
    set(SLANG_LIBRARY_DIR  ${EngineLibDir}/slang/lib/macOS)
    set(SLANG_SHARED_LIBRARY_DIR  ${EngineLibDir}/Toolchains/XcodeDefault.xctoolchain/usr/lib)
    set(CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS} -O3") 
else()
    set(SLANG_LIBRARY_DIR  ${EngineLibDir}/slang/lib/macOS)
    set(SLANG_SHARED_LIBRARY_DIR  ${EngineLibDir}/slang/bin/macOS)
    set(CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS} -O3")

endif()
 
macro (SlangLink target)

    include_directories(${EngineLibDir}/slang/include)

    target_link_libraries(${target} PUBLIC
         slang
    )

    target_link_directories(${target} PUBLIC ${SLANG_LIBRARY_DIR})
    ##使用宏来加载动态库dll1和dll2
#    ADD_DELAYLOAD_FLAGS(${target} slang)

    # copy resources on post build
    add_custom_command(TARGET ${target} POST_BUILD
    # directory
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${SLANG_SHARED_LIBRARY_DIR}
        ${ProjectBuildDir}#[[/${EngineLibRootDir}/ShaderCompiler]]
    )

endmacro()