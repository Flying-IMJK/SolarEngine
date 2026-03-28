

if (WIN32)
    if (CMAKE_BUILD_TYPE STREQUAL Debug)
        set(ISPC_TEXCOMP_LIBRARY_DIR  ${EngineLibDir}/ISPCTextureCompressor/lib/win/debug)
        set(ISPC_TEXCOMP_SHARED_LIBRARY_DIR  ${EngineLibDir}/ISPCTextureCompressor/bin/win/debug)
    else()
        set(ISPC_TEXCOMP_LIBRARY_DIR  ${EngineLibDir}/ISPCTextureCompressor/lib/win/release)
        set(ISPC_TEXCOMP_SHARED_LIBRARY_DIR  ${EngineLibDir}/ISPCTextureCompressor/bin/win/release)
    endif()

elseif(${CMAKE_HOST_SYSTEM_NAME} STREQUAL "Linux")
    set(ISPC_TEXCOMP_LIBRARY_DIR  ${EngineLibDir}/ISPCTextureCompressor/lib/Linux)
    set(ISPC_TEXCOMP_SHARED_LIBRARY_DIR  ${EngineLibDir}/ISPCTextureCompressor/bin/Linux)
    set(CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS} -O3")


elseif(${CMAKE_HOST_SYSTEM_PROCESSOR} STREQUAL "arm64") 
    # link to XCode Toolchains' universal binary libclang.dylib 
    set(ISPC_TEXCOMP_LIBRARY_DIR  ${EngineLibDir}/ISPCTextureCompressor/lib/macOS)
    set(ISPC_TEXCOMP_SHARED_LIBRARY_DIR  ${EngineLibDir}/Toolchains/XcodeDefault.xctoolchain/usr/lib)
    set(CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS} -O3") 
else()
    set(ISPC_TEXCOMP_LIBRARY_DIR  ${EngineLibDir}/ISPCTextureCompressor/lib/macOS)
    set(ISPC_TEXCOMP_SHARED_LIBRARY_DIR  ${EngineLibDir}/ISPCTextureCompressor/bin/macOS)
    set(CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS} -O3")

endif()

 
macro (ISPCTextureCompressorLink target)

    target_include_directories(${target}  
        PUBLIC ${EngineLibDir}/ISPCTextureCompressor
    )

    target_link_libraries(${target} PUBLIC ispc_texcomp )

    ADD_DELAYLOAD_FLAGS(${target} ispc_texcomp)

    target_link_directories(${target} PUBLIC ${ISPC_TEXCOMP_LIBRARY_DIR})

    # copy resources on post build
    add_custom_command(TARGET ${target} POST_BUILD
    # directory
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${ISPC_TEXCOMP_SHARED_LIBRARY_DIR}
        ${ProjectBuildDir}/${EngineLibRootDir}
    )

endmacro()