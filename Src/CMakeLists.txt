
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

###################################

add_subdirectory(Libraries)
add_subdirectory(Reflector/Code)
add_subdirectory(Engine/Core)
#add_subdirectory(Engine/CSharp)
add_subdirectory(Engine/Runtime)
add_subdirectory(Engine/Editor)
add_subdirectory(Applications)


############### 引擎资源拷贝 ###############
add_custom_target(SE.Resources ALL
	# mustache templates directory
	COMMAND ${CMAKE_COMMAND} -E copy_directory
		"${EngineAssetsDir}"
		"${ProjectBuildDir}/Cache/Assets"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${EngineShadersDir}"
        "${ProjectBuildDir}/Shaders"
)

############### 引擎反射 ###############
include(Reflector/Code/precompile.cmake)

############### 引擎代码编译依赖 ###############
add_dependencies(SE.Reflector SE.Core)
add_dependencies(${PRECOMPILE_REBUILD_TARGET} SE.Reflector)
add_dependencies(${PRECOMPILE_CLEAR_TARGET} SE.Reflector)
add_dependencies(${PRECOMPILE_TARGET} SE.Reflector)

#add_dependencies(SE.Runtime SE.Core SE.Resources SE.CSharp)
add_dependencies(SE.Editor SE.Runtime)


