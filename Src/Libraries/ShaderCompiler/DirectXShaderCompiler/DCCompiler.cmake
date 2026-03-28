
# ## 1.7.2308
set(dx_path ${EngineLibDir}/ShaderCompiler/DirectXShaderCompiler)
set(dxc_include ${dx_path}/inc)

if(WIN32)
    if(CMAKE_CL_64)
        set(dxc_lib ${dx_path}/lib/x64/dxcompiler.lib)
        set(dxc_executable ${dx_path}/bin/x64/dxc.exe)
        set(dxc_dll ${dx_path}/bin/x64)
    else()
        set(dxc_lib ${dx_path}/lib/x86/dxcompiler.lib)
        set(dxc_executable ${dx_path}/bin/x86/dxc.exe)
        set(dxc_dll ${dx_path}/bin/x86)
    endif()

elseif(UNIX)
    message(FATAL_ERROR "Unknown Platform")
else()
    message(FATAL_ERROR "Unknown Platform")
endif()


macro (DirectXShaderCompilerCopy target)
    # copy resources on post build
    add_custom_command(TARGET ${target} POST_BUILD
    # file
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${dxc_dll}/dxcompiler.dll
        ${ProjectBuildDir}/${EngineLibRootDir}/ShaderCompiler
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${dxc_dll}/dxil.dll
        ${ProjectBuildDir}/${EngineLibRootDir}/ShaderCompiler
    )
endmacro()