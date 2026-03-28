
add_subdirectory(Generator)

RUNTIME_ADD(
        IShaderAsset.h
        IShaderAsset.cpp
    ShaderStorage.h
    MaterialBase.h
    MaterialBase.cpp
    Material.h
    Material.cpp
    Shader.h
    Shader.cpp
    MaterialInstance.h
    MaterialInstance.cpp
    ShaderCacheManager.h
    ShaderCacheManager.cpp
)