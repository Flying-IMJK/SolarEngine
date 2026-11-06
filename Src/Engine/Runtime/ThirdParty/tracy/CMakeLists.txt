
aux_source_directory("client" clientFile)
aux_source_directory("common" commonFile)
aux_source_directory("libbacktrace" libbacktraceFile)

#[[file(GLOB_RECURSE allSource "*.cpp" "*.c")
file(GLOB_RECURSE allHead "*.hpp" "*.h")]]

RUNTIME_ADD(
        ${clientFile}
        ${commonFile}
#        ${libbacktraceFile}
        tracy/Tracy.hpp
#        TracyClient.cpp
)

if(USE_EDITOR AND USE_PROFILER)
    include(FetchContent)
    # Match the version required by the vendored Tracy server.
    set(BUILD_SHARED_LIBS OFF)
    set(CAPSTONE_BUILD_STATIC_RUNTIME OFF)
    set(CAPSTONE_BUILD_TESTS OFF)
    set(CAPSTONE_BUILD_CSTOOL OFF)
    set(CAPSTONE_BUILD_CSTEST OFF)
    set(CAPSTONE_INSTALL OFF)
    FetchContent_Declare(SETracyCapstone
        GIT_REPOSITORY https://github.com/capstone-engine/capstone.git
        GIT_TAG 5.0.3
        GIT_SHALLOW TRUE
    )
    FetchContent_MakeAvailable(SETracyCapstone)

    set(SE_TRACY_ROOT "${EngineLibDir}/tracy")
    file(GLOB SE_TRACY_ZSTD_SOURCES CONFIGURE_DEPENDS
        "${SE_TRACY_ROOT}/zstd/common/*.c"
        "${SE_TRACY_ROOT}/zstd/compress/*.c"
        "${SE_TRACY_ROOT}/zstd/decompress/*.c"
        "${SE_TRACY_ROOT}/zstd/dictBuilder/*.c"
    )
    add_library(SETracyZstd STATIC ${SE_TRACY_ZSTD_SOURCES})
    target_compile_definitions(SETracyZstd PRIVATE ZSTD_DISABLE_ASM)
    target_include_directories(SETracyZstd PUBLIC "${SE_TRACY_ROOT}/zstd")

    # Runtime already owns the identical Tracy common sources. Do not compile them twice.
    add_library(SETracyServer STATIC
        "${SE_TRACY_ROOT}/server/TracyMemory.cpp"
        "${SE_TRACY_ROOT}/server/TracyMmap.cpp"
        "${SE_TRACY_ROOT}/server/TracyPrint.cpp"
        "${SE_TRACY_ROOT}/server/TracySysUtil.cpp"
        "${SE_TRACY_ROOT}/server/TracyTaskDispatch.cpp"
        "${SE_TRACY_ROOT}/server/TracyTextureCompression.cpp"
        "${SE_TRACY_ROOT}/server/TracyThreadCompress.cpp"
        "${SE_TRACY_ROOT}/server/TracyWorker.cpp"
    )
    target_include_directories(SETracyServer PUBLIC "${SE_TRACY_ROOT}/server")
    target_include_directories(SETracyServer PRIVATE "${setracycapstone_SOURCE_DIR}/include/capstone")
    target_compile_definitions(SETracyServer PRIVATE TRACY_ENABLE TRACY_EXPORTS TRACY_DBGHELP_LOCK=SEDbgHelp)
    target_link_libraries(SETracyServer PRIVATE SETracyZstd capstone)
    if(MSVC)
        target_compile_options(SETracyServer PRIVATE /bigobj)
    endif()
    set_target_properties(SETracyServer SETracyZstd capstone PROPERTIES FOLDER "ThirdParty/Tracy")
endif()
