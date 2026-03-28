

if (WIN32)

    if (CMAKE_BUILD_TYPE STREQUAL Debug)
        set(NETWORKING_LIBRARY_DIR  ${EngineLibDir}/GameNetworkingSockets/debug/lib)
        set(NETWORKING_SHARED_LIBRARY_DIR  ${EngineLibDir}/GameNetworkingSockets/debug/bin)
    else()
        set(NETWORKING_LIBRARY_DIR  ${EngineLibDir}/GameNetworkingSockets/release/lib)
        set(NETWORKING_SHARED_LIBRARY_DIR  ${EngineLibDir}/GameNetworkingSockets/release/bin)
    endif()

elseif(${CMAKE_HOST_SYSTEM_NAME} STREQUAL "Linux")
    set(NETWORKING_LIBRARY_DIR  ${EngineLibDir}/GameNetworkingSockets/lib/Linux)
    set(NETWORKING_SHARED_LIBRARY_DIR  ${EngineLibDir}/GameNetworkingSockets/bin/Linux)
    set(CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS} -O3")


elseif(${CMAKE_HOST_SYSTEM_PROCESSOR} STREQUAL "arm64") 
    # link to XCode Toolchains' universal binary libclang.dylib 
    set(NETWORKING_LIBRARY_DIR  ${EngineLibDir}/GameNetworkingSockets/lib/macOS)
    set(NETWORKING_SHARED_LIBRARY_DIR  ${EngineLibDir}/Toolchains/XcodeDefault.xctoolchain/usr/lib)
    set(CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS} -O3") 
else()
    set(NETWORKING_LIBRARY_DIR  ${EngineLibDir}/GameNetworkingSockets/lib/macOS)
    set(NETWORKING_SHARED_LIBRARY_DIR  ${EngineLibDir}/GameNetworkingSockets/bin/macOS)
    set(CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS} -O3")

endif()
 
macro (GameNetworkingSocketsLink target)

    include_directories(${EngineLibDir}/GameNetworkingSockets/include/GameNetworkingSockets)

    target_link_libraries(${target} PUBLIC
        GameNetworkingSockets
    )

    target_link_directories(${target} PUBLIC ${NETWORKING_LIBRARY_DIR})
    ##使用宏来加载动态库dll1和dll2
    ADD_DELAYLOAD_FLAGS(${target} GameNetworkingSockets legacy libcrypto-3-x64 libprotobufd libssl-3-x64)

    # copy resources on post build
    add_custom_command(TARGET ${target} POST_BUILD
    # directory
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${NETWORKING_SHARED_LIBRARY_DIR}
        ${ProjectBuildDir}/${EngineLibRootDir}/GameNetworkingSocket
    )

endmacro()

macro (GameNetworkingSocketsLinkReflector target)
    # copy resources on post build
    add_custom_command(TARGET ${target} POST_BUILD
    # directory
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${NETWORKING_SHARED_LIBRARY_DIR}
        $<TARGET_FILE_DIR:${target}>
    )

endmacro()