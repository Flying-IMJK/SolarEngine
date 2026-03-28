
add_subdirectory(Parser)

RUNTIME_ADD(
    ShaderCompiler.h
    ShaderCompiler.cpp
    ShaderCompilationContext.h
    ShadersCompilation.h
    ShadersCompilation.cpp

    Vulkan/ShaderCompilerVulkan.h
    Vulkan/ShaderCompilerVulkan.cpp
)

