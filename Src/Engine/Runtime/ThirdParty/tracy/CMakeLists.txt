
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