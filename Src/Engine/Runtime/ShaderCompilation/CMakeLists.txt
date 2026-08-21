
add_subdirectory(Parser)
add_subdirectory(Slang)

RUNTIME_ADD(
    ShaderCompileTypes.h
    ShaderCompileTypes.cpp
    ShaderCompiler.h
    ShaderCompiler.cpp
    ShaderCompilationContext.h
    ShadersCompilation.h
    ShadersCompilation.cpp

    Vulkan/ShaderCompilerVulkan.h
    Vulkan/ShaderCompilerVulkan.cpp
)

