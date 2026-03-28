cmake_minimum_required (VERSION 3.8)

# 设置工程名 使用语言c/c++
#project(ElectronLibraries LANGUAGES CXX C)

set(CMAKE_C_STANDARD 11)            # 使用c 11
set(CMAKE_CXX_STANDARD 17)          # 使用c++ 17
set(CMAKE_CXX_STANDARD_REQUIRED ON) # 指定所选标准版本，如果版本不可以，CMake停止配置并出错
set(CMAKE_C_EXTENSIONS ON)          # 支持ISO C++标准，不支持特定编译器扩展
set(CMAKE_CXX_EXTENSIONS ON)

include(cmake/Tools.cmake)

# 性能分析器
#[[if(NOT TARGET TracyClient)
    set(TRACY_STATIC ON CACHE BOOL "" FORCE)
    add_subdirectory(tracy)

    set_target_properties(TracyClient PROPERTIES FOLDER ${EngineLibDir}/tracy)

    if (WIN32)
        set_target_properties(TracyClient PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS  True)
    endif()
    NoWarning(TracyClient)
endif()]]

if(NOT TARGET ShaderCompiler)
    add_subdirectory(ShaderCompiler)
    set_target_properties(ShaderCompiler PROPERTIES FOLDER ${EngineLibDir}/ShaderCompiler)
endif()

# version 9.1.0
if(NOT TARGET fmt)
    add_subdirectory(fmt)
    set_target_properties(fmt PROPERTIES FOLDER ${EngineLibDir}/Fmt)
    NoWarning(fmt)
endif()

if(NOT TARGET sqlite3)
    add_subdirectory(sqlite3)

    set_target_properties(sqlite3 PROPERTIES FOLDER ${EngineLibDir}/Sqlite3)

    if (WIN32)
    set_target_properties(sqlite3 PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS  True)
    endif()
    NoWarning(sqlite3)
endif()

if(NOT TARGET freetype)
    set(FT_DISABLE_PNG ON CACHE BOOL "" FORCE)
    set(FT_DISABLE_ZLIB ON CACHE BOOL "" FORCE)
    set(FT_DISABLE_BZIP2 ON CACHE BOOL "" FORCE)
    set(FT_DISABLE_HARFBUZZ ON CACHE BOOL "" FORCE)
    set(FT_DISABLE_BROTLI ON CACHE BOOL "" FORCE)
    
    add_subdirectory(Freetype)

    set_target_properties(freetype PROPERTIES FOLDER ${EngineLibDir}/Freetype)

    # if (WIN32)
    # set_target_properties(sqlite3 PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS True)
    # endif()
    NoWarning(freetype)
endif()


# version 5.2.4
if(NOT TARGET assimp)
#    set(ASSIMP_LIBRARY_OUTPUT_DIRECTORY ${ProjectBuildDir}/${EngineLibRootDir}/Assimp)
#    set(ASSIMP_RUNTIME_OUTPUT_DIRECTORY ${ProjectBuildDir}/${EngineLibRootDir}/Assimp)

    # 禁用所有导出器（Exporter）
    set(ASSIMP_NO_EXPORT ON CACHE BOOL "" FORCE)             # 完全禁用导出功能

    # 只启用 FBX 、OBJ、GLTF 格式
    set(ASSIMP_BUILD_FBX_IMPORTER ON CACHE BOOL "" FORCE)
    set(ASSIMP_BUILD_OBJ_IMPORTER ON CACHE BOOL "" FORCE)
    set(ASSIMP_BUILD_GLTF_IMPORTER ON CACHE BOOL "" FORCE)

    set(ASSIMP_BUILD_AMF_IMPORTER OFF CACHE BOOL "" FORCE)
    set(ASSIMP_BUILD_3DS_IMPORTER OFF CACHE BOOL "" FORCE)
    set(ASSIMP_BUILD_AC_IMPORTER OFF CACHE BOOL "" FORCE)
    set(ASSIMP_BUILD_ASSBIN_IMPORTER OFF CACHE BOOL "" FORCE)
    set(ASSIMP_BUILD_B3D_IMPORTER OFF CACHE BOOL "" FORCE)
    set(ASSIMP_BUILD_ASE_IMPORTER OFF CACHE BOOL "" FORCE)
    set(ASSIMP_BUILD_BVH_IMPORTER OFF CACHE BOOL "" FORCE)
    set(ASSIMP_BUILD_DXF_IMPORTER OFF CACHE BOOL "" FORCE)
    set(ASSIMP_BUILD_CSM_IMPORTER OFF CACHE BOOL "" FORCE)
    set(ASSIMP_BUILD_HMP_IMPORTER OFF CACHE BOOL "" FORCE)
    set(ASSIMP_BUILD_IRRMESH_IMPORTER OFF CACHE BOOL "" FORCE)
    set(ASSIMP_BUILD_IQM_IMPORTER OFF CACHE BOOL "" FORCE)
    set(ASSIMP_BUILD_IRR_IMPORTER OFF CACHE BOOL "" FORCE)
    set(ASSIMP_BUILD_LWO_IMPORTER OFF CACHE BOOL "" FORCE)
    set(ASSIMP_BUILD_LWS_IMPORTER OFF CACHE BOOL "" FORCE)
    set(ASSIMP_BUILD_M3D_IMPORTER OFF CACHE BOOL "" FORCE)
    set(ASSIMP_BUILD_COLLADA_IMPORTER OFF CACHE BOOL "" FORCE)
    set(ASSIMP_BUILD_MD2_IMPORTER OFF CACHE BOOL "" FORCE)
    set(ASSIMP_BUILD_MD3_IMPORTER OFF CACHE BOOL "" FORCE)
    set(ASSIMP_BUILD_MD5_IMPORTER OFF CACHE BOOL "" FORCE)
    set(ASSIMP_BUILD_MDC_IMPORTER OFF CACHE BOOL "" FORCE)
    set(ASSIMP_BUILD_MDL_IMPORTER OFF CACHE BOOL "" FORCE)
    set(ASSIMP_BUILD_NFF_IMPORTER OFF CACHE BOOL "" FORCE)
    set(ASSIMP_BUILD_NDO_IMPORTER OFF CACHE BOOL "" FORCE)
    set(ASSIMP_BUILD_OFF_IMPORTER OFF CACHE BOOL "" FORCE)
    set(ASSIMP_BUILD_OGRE_IMPORTER OFF CACHE BOOL "" FORCE)
    set(ASSIMP_BUILD_OPENGEX_IMPORTER OFF CACHE BOOL "" FORCE)
    set(ASSIMP_BUILD_PLY_IMPORTER OFF CACHE BOOL "" FORCE)
    set(ASSIMP_BUILD_MS3D_IMPORTER OFF CACHE BOOL "" FORCE)
    set(ASSIMP_BUILD_COB_IMPORTER OFF CACHE BOOL "" FORCE)
    set(ASSIMP_BUILD_BLEND_IMPORTER OFF CACHE BOOL "" FORCE)
    set(ASSIMP_BUILD_IFC_IMPORTER OFF CACHE BOOL "" FORCE)
    set(ASSIMP_BUILD_XGL_IMPORTER OFF CACHE BOOL "" FORCE)
    set(ASSIMP_BUILD_Q3D_IMPORTER OFF CACHE BOOL "" FORCE)
    set(ASSIMP_BUILD_Q3BSP_IMPORTER OFF CACHE BOOL "" FORCE)
    set(ASSIMP_BUILD_RAW_IMPORTER OFF CACHE BOOL "" FORCE)
    set(ASSIMP_BUILD_SIB_IMPORTER OFF CACHE BOOL "" FORCE)
    set(ASSIMP_BUILD_SMD_IMPORTER OFF CACHE BOOL "" FORCE)
    set(ASSIMP_BUILD_STL_IMPORTER OFF CACHE BOOL "" FORCE)
    set(ASSIMP_BUILD_TERRAGEN_IMPORTER OFF CACHE BOOL "" FORCE)
    set(ASSIMP_BUILD_3D_IMPORTER OFF CACHE BOOL "" FORCE)
    set(ASSIMP_BUILD_X_IMPORTER OFF CACHE BOOL "" FORCE)
    set(ASSIMP_BUILD_3MF_IMPORTER OFF CACHE BOOL "" FORCE)
    set(ASSIMP_BUILD_MMD_IMPORTER OFF CACHE BOOL "" FORCE)
    set(ASSIMP_BUILD_X3D_IMPORTER OFF CACHE BOOL "" FORCE)


    set(BUILD_SHARED_LIBS ON CACHE BOOL "" FORCE)
    set(ASSIMP_BUILD_FRAMEWORK OFF CACHE BOOL "" FORCE) #"Build package as Mac OS X Framework bundle."
    set(ASSIMP_DOUBLE_PRECISION OFF CACHE BOOL "" FORCE) #"Set to ON to enable double precision processing"
    set(ASSIMP_OPT_BUILD_PACKAGES OFF CACHE BOOL "" FORCE) #"Set to ON to generate CPack configuration files and packaging targets"
    set(ASSIMP_ANDROID_JNIIOSYSTEM OFF CACHE BOOL "" FORCE) #"Android JNI IOSystem support is active"
    set(ASSIMP_BUILD_ZLIB ON CACHE BOOL "" FORCE) #"Build your own zlib"
    set(ASSIMP_BUILD_ASSIMP_TOOLS OFF CACHE BOOL "" FORCE) #"If the supplementary tools for Assimp are built in addition to the library."
    set(ASSIMP_BUILD_SAMPLES OFF CACHE BOOL "" FORCE) #"If the official samples are built as well (needs Glut)."
    set(ASSIMP_BUILD_TESTS OFF CACHE BOOL "" FORCE) #"If the test suite for Assimp is built in addition to the library."
    set(ASSIMP_COVERALLS OFF CACHE BOOL "" FORCE) #"Enable this to measure test coverage."
    set(ASSIMP_INSTALL OFF CACHE BOOL "" FORCE) #"Disable this if you want to use assimp as a submodule."
    set(ASSIMP_ERROR_MAX OFF CACHE BOOL "" FORCE) #"Enable all warnings."
    set(ASSIMP_ASAN OFF CACHE BOOL "" FORCE) #"Enable AddressSanitizer."
    set(ASSIMP_UBSAN OFF CACHE BOOL "" FORCE) #"Enable Undefined Behavior sanitizer."
    set(ASSIMP_BUILD_DOCS OFF CACHE BOOL "" FORCE) # "Build documentation using Doxygen."
    set(ASSIMP_INJECT_DEBUG_POSTFIX OFF CACHE BOOL "" FORCE) #"Inject debug postfix in .a/.so/.dll lib names"
    set(ASSIMP_IGNORE_GIT_HASH OFF CACHE BOOL "" FORCE) #"Don't call git to get the hash."
    add_subdirectory(assimp)
    set_target_properties(assimp PROPERTIES FOLDER ${EngineLibDir}/Assimp)
    NoWarning(assimp)
endif()