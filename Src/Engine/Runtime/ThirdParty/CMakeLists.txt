
add_subdirectory(stb)
add_subdirectory(vulkan)
add_subdirectory(detex)
add_subdirectory(meshoptimizer)
add_subdirectory(rapidjson)
add_subdirectory(tracy)

RUNTIME_ADD(
    LZ4/lz4.h
    LZ4/lz4.cpp

    bc7enc16/bc7enc16.h
    bc7enc16/bc7enc16.cpp

    tinyexr/tinyexr.h
    tinyexr/miniz.h

    xxhash/xxhash.h
    xxhash/xxhash.c

    concurrentqueue/lightweightsemaphore.h
    concurrentqueue/concurrentqueue.h
    concurrentqueue/blockingconcurrentqueue.h

    iniparser/iniparser.h
    iniparser/iniparser.c
    iniparser/dictionary.h
    iniparser/dictionary.c

    mpack/mpack.h
    mpack/mpack.c
)