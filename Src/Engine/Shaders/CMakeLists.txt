############### 引擎 Shader 目标 ###############

file(GLOB_RECURSE SE_SHADER_FILES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.hlsl"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.shader"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.slang"
)

# SEShader 仅用于 IDE 文件组织和资源复制，Shader 文件不参与构建系统编译。
set_source_files_properties(${SE_SHADER_FILES}
    PROPERTIES HEADER_FILE_ONLY TRUE
)

# Visual Studio 会按扩展名把 .hlsl 映射为 FXCompile，需要显式改为展示项。
if(CMAKE_GENERATOR MATCHES "^Visual Studio")
    set_source_files_properties(${SE_SHADER_FILES}
        PROPERTIES
            VS_TOOL_OVERRIDE "None"
            VS_SETTINGS "ExcludedFromBuild=true"
    )
endif()

# 在 Visual Studio 中按 Engine/Shaders 目录结构展示所有 Shader 文件。
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" PREFIX "Shaders" FILES ${SE_SHADER_FILES})

add_custom_target(SEShader ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${EngineAssetsDir}"
        "${ProjectBuildDir}/Cache/Assets"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${EngineShadersDir}"
        "${ProjectBuildDir}/Shaders"
    SOURCES ${SE_SHADER_FILES}
)
