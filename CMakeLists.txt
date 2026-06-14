
cmake_minimum_required (VERSION 3.26)

# # 设置工程名 使用语言c/c++
project(SolarEngine LANGUAGES CXX C)

set(CMAKE_C_STANDARD 11)            # 使用c 11
set(CMAKE_CXX_STANDARD 17)          # 使用c++ 17
set(CMAKE_CXX_STANDARD_REQUIRED ON) # 指定所选标准版本，如果版本不可以，CMake停止配置并出错
set(CMAKE_C_EXTENSIONS OFF)         # 支持ISO C++标准，不支持特定编译器扩展
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(CMakeDependentOption)


option(USE_EDITOR "构建编辑器" ON)
option(USE_DEVELOPMENT "开发模式" ON)
option(USE_PROFILER "性能分析器" ON)

# 工程根目录
set(ProjectRootDir ${CMAKE_CURRENT_SOURCE_DIR})

# 构建目录
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_definitions(-DENGINE_DEVELOPMENT_TOOLS)
	set(ProjectBuildDir "${ProjectRootDir}/Debug")
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${ProjectBuildDir}")
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${ProjectBuildDir}")
elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
	set(ProjectBuildDir "${ProjectRootDir}/Release")
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${ProjectBuildDir}")
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${ProjectBuildDir}")
endif()


set(EngineRootDir "${ProjectRootDir}/Src")      # 引擎根目录

############### 设置引擎库文件 ###############
set(EngineLibDir  "${EngineRootDir}/Libraries") # 三方库
set(EngineDir     "${EngineRootDir}/Engine")


############### 设置引擎模块 ###############
set(EngineAssetsDir  "${EngineDir}/Assets")    # 引擎资源
set(EngineShadersDir "${EngineDir}/Shaders")   # 引擎Shader
set(EngineCoreDir    "${EngineDir}/Core")      # 核心
set(EngineRuntimeDir "${EngineDir}/Runtime")   # 运行时
set(EngineEditorDir  "${EngineDir}/Editor")    # 编辑器

############### 设置Lib ###############
set(EngineLibRootDir  "Lib")  # DLL 存放根路径
# DLL 搜索路径列表
set(EngineLoadLibDirList "${EngineLibRootDir},${EngineLibRootDir}/GameNetworkingSocket,${EngineLibRootDir}/ShaderCompiler,${EngineLibRootDir}/Assimp")

if (USE_PROFILER)
    add_compile_definitions(SE_PROFILER)
endif ()
if (USE_EDITOR)
    add_compile_definitions(SE_EDITOR)
endif ()
if (USE_DEVELOPMENT)
    add_compile_definitions(SE_DEVELOPMENT)
endif ()

add_compile_definitions(EngineLoadLibDirList="${EngineLoadLibDirList}")

set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib;${ProjectBuildDir}")


add_subdirectory(Src)
