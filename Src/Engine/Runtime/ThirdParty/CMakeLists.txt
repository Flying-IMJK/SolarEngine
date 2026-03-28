
add_subdirectory(stb)
add_subdirectory(vulkan)
add_subdirectory(detex)
add_subdirectory(meshoptimizer)
add_subdirectory(netCore)

RUNTIME_ADD(
    LZ4/lz4.h
    LZ4/lz4.cpp

    bc7enc16/bc7enc16.h
    bc7enc16/bc7enc16.cpp

    tinyexr/tinyexr.h
    tinyexr/miniz.h
)