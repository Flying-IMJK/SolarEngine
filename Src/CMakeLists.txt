
############### 功能宏 ###############

## 反射预编译模块
MACRO(PRECOMPILE_PROJECT target targetName)
    set_target_properties(${target} PROPERTIES PrecompileTargetName ${targetName})
    set_target_properties(${target} PROPERTIES TargetDir ${CMAKE_CURRENT_SOURCE_DIR})

    string(REGEX REPLACE ".*/(.*)" "\\1" CURRENT_FOLDER ${CMAKE_CURRENT_SOURCE_DIR})

    set(TargetIncludeList)
    foreach(str ${ARGN})
        string(REPLACE "${CMAKE_CURRENT_SOURCE_DIR}/" "${CURRENT_FOLDER}/" strTemp ${str})
        list(APPEND TargetIncludeList ${strTemp})
    endforeach(str)
    set_target_properties(${target} PROPERTIES TargetInclude "${TargetIncludeList}")
ENDMACRO()

##定义延迟加载的宏
MACRO(ADD_DELAYLOAD_FLAGS target)

    FOREACH(dll ${ARGN})
        target_link_options(${target} PRIVATE "/DELAYLOAD:${dll}.dll")
    ENDFOREACH()
ENDMACRO()

MACRO(SET_TARGET_DLL_OUTPUT_DIR target dir)
    set_target_properties(${target} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${dir}"
    )
ENDMACRO()

#CSharp Target
MACRO(CSHARP_TARGET target configFile)
    set(CSHARP_INTERMEDIATE_DIR "${ProjectRootDir}/Intermediate/${target}")
    set(CSHARP_OUTPUT_DLL "${ProjectBuildDir}/${target}CSharp.dll")

    if(CMAKE_GENERATOR MATCHES "Visual Studio")
        set(CSHARP_VS_PROJECT_TARGET "${target}CSharp")

        include_external_msproject(
            ${CSHARP_VS_PROJECT_TARGET}
            "${configFile}"
            TYPE 9A19103F-16F7-4668-BE54-9A1E7A4F7556F
            PLATFORM AnyCPU
        )

        set_target_properties(${CSHARP_VS_PROJECT_TARGET} PROPERTIES
            EXCLUDE_FROM_ALL TRUE
            EXCLUDE_FROM_DEFAULT_BUILD TRUE
        )
    endif()

    add_custom_target(${target}.CSharp ALL
            COMMAND ${CMAKE_COMMAND} -E env DOTNET_CLI_UI_LANGUAGE=en dotnet build
            "${configFile}"
            --configuration ${CSHARP_CONFIGURATION}
            --output        "${ProjectBuildDir}"
            -p:BaseIntermediateOutputPath="${CSHARP_INTERMEDIATE_DIR}/obj/"
            -p:MSBuildProjectExtensionsPath="${CSHARP_INTERMEDIATE_DIR}/obj/"
            -p:BaseOutputPath="${CSHARP_INTERMEDIATE_DIR}/bin/"
            --nologo
            -v minimal
            ${CSHARP_EDITOR_FLAG}
            BYPRODUCTS "${CSHARP_OUTPUT_DLL}"
            COMMENT "Building ${target} (dotnet build)..."
            VERBATIM
    )
    set_target_properties(${target}.CSharp PROPERTIES FOLDER "Tool")
ENDMACRO()

############### 配置 ###############

add_compile_options(/wd4251)

add_definitions("-DUNICODE" "-D_UNICODE")

if (MSVC)
    # 添加编译选项, 设置 输入代码的编码格式为 utf-8
    add_compile_options("/source-charset:utf-8")
endif()

# 平台定义
if (WIN32)
    add_definitions(-DPLATFORM_WIN32)
    add_definitions(-DPLATFORM_WINDOWS)
endif ()

if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    add_compile_options("/MP")
    add_definitions("-D_SILENCE_STDEXT_ARR_ITERS_DEPRECATION_WARNING")
endif()

# 将 CMake 的 USE_EDITOR 选项传递给 dotnet build
if(USE_EDITOR)
    set(CSHARP_EDITOR_FLAG "/p:SE_EDITOR=true")
else()
    set(CSHARP_EDITOR_FLAG "/p:SE_EDITOR=false")
endif()

# Debug / Release 配置映射
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(CSHARP_CONFIGURATION "Debug")
else()
    set(CSHARP_CONFIGURATION "Release")
endif()

###################################

add_subdirectory(Libraries)
add_subdirectory(BuildTool/Code)
add_subdirectory(Engine/Runtime)
add_subdirectory(Engine/Editor)
add_subdirectory(Applications)


############### 引擎资源拷贝 ###############
add_custom_target(SEResources ALL
	# mustache templates directory
	COMMAND ${CMAKE_COMMAND} -E copy_directory
		"${EngineAssetsDir}"
		"${ProjectBuildDir}/Cache/Assets"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${EngineShadersDir}"
        "${ProjectBuildDir}/Shaders"
)
set_target_properties(SEResources PROPERTIES FOLDER "Tool")

############### 引擎Target 依赖 ###############

add_dependencies(SERuntime SEResources)
add_dependencies(SEEditor SERuntime)

############### 引擎预构建 ###############
include(BuildTool/Code/precompile.cmake)