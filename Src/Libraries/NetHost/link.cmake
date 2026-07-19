

set(NETHOST_DIR      ${EngineLibDir}/NetHost)


if (WIN32)
    # 对于 nethost，Debug 和 Release 使用相同的库
    set(NETHOST_LIBRARY_DIR      ${NETHOST_DIR}/lib/win64)
    set(NETHOST_SHARED_LIBRARY_DIR ${NETHOST_DIR}/bin/win64)
elseif(${CMAKE_HOST_SYSTEM_NAME} STREQUAL "Linux")
    set(NETHOST_LIBRARY_DIR      ${NETHOST_DIR}/lib/Linux)
    set(NETHOST_SHARED_LIBRARY_DIR ${NETHOST_DIR}/bin/Linux)
else()
    set(NETHOST_LIBRARY_DIR      ${NETHOST_DIR}/lib/macOS)
    set(NETHOST_SHARED_LIBRARY_DIR ${NETHOST_DIR}/bin/macOS)
endif()

# 8.0.22
macro (NetCoreLink target)

    target_include_directories(${target} PRIVATE ${NETHOST_DIR}/Include)

    target_link_libraries(${target} PUBLIC
            nethost
    )

    target_link_directories(${target} PUBLIC ${NETHOST_LIBRARY_DIR})

    ##使用宏来加载动态库
    ADD_DELAYLOAD_FLAGS(${target} nethost)

    # 复制 nethost.dll 到构建目录
    add_custom_command(TARGET ${target} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${NETHOST_SHARED_LIBRARY_DIR}/nethost${CMAKE_SHARED_LIBRARY_SUFFIX}
            ${ProjectBuildDir}/${EngineLibRootDir}
    )
endmacro()