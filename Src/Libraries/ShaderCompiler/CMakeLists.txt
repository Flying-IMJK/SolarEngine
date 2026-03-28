
project(ShaderCompiler LANGUAGES CXX C)

include(DirectXShaderCompiler/DCCompiler.cmake)

# 12.3.1
if(NOT TARGET glslang)
    set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
    set(ENABLE_CTEST OFF CACHE BOOL "" FORCE)
    set(BUILD_EXTERNAL ON CACHE BOOL "" FORCE)

    add_subdirectory(glslang)
    set_target_properties(glslang PROPERTIES FOLDER ${EngineLibDir}/Glslang)
endif()

# version 1.1.5
if(NOT TARGET spirv-cross)
    # set(SPIRV_CROSS_STATIC ON)
    # set(SPIRV_CROSS_SHARED OFF)
    set(SPIRV_CROSS_ENABLE_TESTS OFF CACHE BOOL "" FORCE)
    set(SPIRV_CROSS_SKIP_INSTALL On CACHE BOOL "" FORCE)
    # set(SPIRV_CROSS_CLI ON)

    add_subdirectory(SPIRV-Cross)
    set_target_properties(spirv-cross PROPERTIES FOLDER ${EngineLibDir}/SPIRV-Cross)
    NoWarning(spirv-cross)
endif()

set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)

add_library(${PROJECT_NAME} SHARED
    $<TARGET_OBJECTS:glslang>
    $<TARGET_OBJECTS:glslang-default-resource-limits>
)
target_include_directories(${PROJECT_NAME}
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/glslang
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/glslang/External/spirv-tools/include
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/DirectXShaderCompiler/inc
)
target_link_libraries(${PROJECT_NAME} PUBLIC ${dxc_lib})
target_link_libraries(${PROJECT_NAME} PUBLIC SPIRV
        spirv-cross-core
        spirv-cross-glsl
        spirv-cross-hlsl 
        spirv-cross-cpp
        spirv-cross-reflect
        spirv-cross-msl
        spirv-cross-util
)
if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    target_link_libraries(${PROJECT_NAME} PUBLIC delayimp )
endif()

SET_TARGET_DLL_OUTPUT_DIR(${PROJECT_NAME} ${ProjectBuildDir}/${EngineLibRootDir}/ShaderCompiler)