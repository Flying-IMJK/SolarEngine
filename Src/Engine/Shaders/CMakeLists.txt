############### 引擎 Shader 目标 ###############

file(GLOB_RECURSE SE_SHADER_FILES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.hlsl"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.shader"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.slang"
)

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
