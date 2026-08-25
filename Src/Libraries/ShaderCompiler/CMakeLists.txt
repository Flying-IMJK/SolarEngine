
project(ShaderCompiler LANGUAGES CXX C)

include(DirectXShaderCompiler/DCCompiler.cmake)

# 12.3.1
if(NOT TARGET glslang)
    set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
    set(ENABLE_CTEST OFF CACHE BOOL "" FORCE)
    set(BUILD_EXTERNAL ON CACHE BOOL "" FORCE)

    set(SE_PREVIOUS_CMAKE_FOLDER "${CMAKE_FOLDER}")
    set(CMAKE_FOLDER "ThirdParty/ShaderCompiler/glslang")
    add_subdirectory(glslang)
    set(CMAKE_FOLDER "${SE_PREVIOUS_CMAKE_FOLDER}")
    set_target_properties(glslang PROPERTIES FOLDER "ThirdParty/ShaderCompiler/glslang")

    foreach(SE_GLSLANG_TARGET IN ITEMS
        GenericCodeGen
        MachineIndependent
        OGLCompiler
        OSDependent
        SPIRV
        SPVRemapper
        glslang
        glslang-default-resource-limits
    )
        if(TARGET ${SE_GLSLANG_TARGET})
            set_target_properties(${SE_GLSLANG_TARGET} PROPERTIES FOLDER "ThirdParty/ShaderCompiler/glslang")
        endif()
    endforeach()

    foreach(SE_GLSLANG_TARGET IN ITEMS
        HLSL
    )
        if(TARGET ${SE_GLSLANG_TARGET})
            set_target_properties(${SE_GLSLANG_TARGET} PROPERTIES FOLDER "ThirdParty/ShaderCompiler/glslang/hlsl")
        endif()
    endforeach()

    foreach(SE_GLSLANG_TARGET IN ITEMS
        glslang-standalone
        spirv-remap
    )
        if(TARGET ${SE_GLSLANG_TARGET})
            set_target_properties(${SE_GLSLANG_TARGET} PROPERTIES FOLDER "ThirdParty/ShaderCompiler/glslang/tools")
        endif()
    endforeach()

    foreach(SE_SPIRV_TOOLS_TARGET IN ITEMS
        SPIRV-Tools-diff
        SPIRV-Tools-link
        SPIRV-Tools-lint
        SPIRV-Tools-opt
        SPIRV-Tools-reduce
        SPIRV-Tools-shared
        SPIRV-Tools-static
    )
        if(TARGET ${SE_SPIRV_TOOLS_TARGET})
            set_target_properties(${SE_SPIRV_TOOLS_TARGET} PROPERTIES FOLDER "ThirdParty/ShaderCompiler/SPIRV-Tools/libraries")
        endif()
    endforeach()

    foreach(SE_SPIRV_TOOLS_TARGET IN ITEMS
        core_tables
        enum_string_mapping
        extinst_tables
        spirv-tools-build-version
        spirv-tools-header-DebugInfo
        spirv-tools-header-NonSemanticShaderDebugInfo100
        spirv-tools-header-OpenCLDebugInfo100
        spv-tools-cldi100
        spv-tools-clspvreflection
        spv-tools-debuginfo
        spv-tools-shdi100
        spv-tools-spv-amd-gs
        spv-tools-spv-amd-sb
        spv-tools-spv-amd-sevp
        spv-tools-spv-amd-stm
    )
        if(TARGET ${SE_SPIRV_TOOLS_TARGET})
            set_target_properties(${SE_SPIRV_TOOLS_TARGET} PROPERTIES FOLDER "ThirdParty/ShaderCompiler/SPIRV-Tools/build")
        endif()
    endforeach()

    foreach(SE_SPIRV_TOOLS_TARGET IN ITEMS
        spirv-as
        spirv-cfg
        spirv-diff
        spirv-dis
        spirv-link
        spirv-lint
        spirv-objdump
        spirv-opt
        spirv-reduce
        spirv-val
    )
        if(TARGET ${SE_SPIRV_TOOLS_TARGET})
            set_target_properties(${SE_SPIRV_TOOLS_TARGET} PROPERTIES FOLDER "ThirdParty/ShaderCompiler/SPIRV-Tools/executables")
        endif()
    endforeach()

    foreach(SE_SPIRV_TOOLS_TARGET IN ITEMS
        spirv-tools-cpp-example
    )
        if(TARGET ${SE_SPIRV_TOOLS_TARGET})
            set_target_properties(${SE_SPIRV_TOOLS_TARGET} PROPERTIES FOLDER "ThirdParty/ShaderCompiler/SPIRV-Tools/examples")
        endif()
    endforeach()

    foreach(SE_SPIRV_TOOLS_TARGET IN ITEMS
        spirv-tools-vimsyntax
    )
        if(TARGET ${SE_SPIRV_TOOLS_TARGET})
            set_target_properties(${SE_SPIRV_TOOLS_TARGET} PROPERTIES FOLDER "ThirdParty/ShaderCompiler/SPIRV-Tools/utilities")
        endif()
    endforeach()

    foreach(SE_SPIRV_TOOLS_TARGET IN ITEMS
        spirv-tools-pkg-config
        spirv-tools-shared-pkg-config
    )
        if(TARGET ${SE_SPIRV_TOOLS_TARGET})
            set_target_properties(${SE_SPIRV_TOOLS_TARGET} PROPERTIES FOLDER "ThirdParty/ShaderCompiler/SPIRV-Tools")
        endif()
    endforeach()
endif()

# version 1.1.5
if(NOT TARGET spirv-cross)
    # set(SPIRV_CROSS_STATIC ON)
    # set(SPIRV_CROSS_SHARED OFF)
    set(SPIRV_CROSS_ENABLE_TESTS OFF CACHE BOOL "" FORCE)
    set(SPIRV_CROSS_SKIP_INSTALL On CACHE BOOL "" FORCE)
    # set(SPIRV_CROSS_CLI ON)

    set(SE_PREVIOUS_CMAKE_FOLDER "${CMAKE_FOLDER}")
    set(CMAKE_FOLDER "ThirdParty/ShaderCompiler/SPIRV-Cross")
    add_subdirectory(SPIRV-Cross)
    set(CMAKE_FOLDER "${SE_PREVIOUS_CMAKE_FOLDER}")
    set_target_properties(spirv-cross PROPERTIES FOLDER "ThirdParty/ShaderCompiler/SPIRV-Cross")
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
