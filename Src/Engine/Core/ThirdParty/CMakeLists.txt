
add_subdirectory(rapidjson)
add_subdirectory(Imgui)
add_subdirectory(tracy)

CORE_ADD(
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