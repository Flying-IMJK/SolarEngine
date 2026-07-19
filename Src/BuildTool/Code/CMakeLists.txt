#cmake_policy(SET CMP0074 OLD)
set(REFLECTOR_TARGET SEBuildTool)

#set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/../CMake")

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

set(LLVM "LLVM15")

set("SOURCES"
    Reflector.h
    Reflector.cpp
    Core/Utils.h
    Core/Utils.cpp
    Core/TopologicalSort.h
    Core/TopologicalSort.cpp
    Core/Time.h
    Core/StringID.h
    Core/String.h
    Core/FileSystem.h
    Core/Dictionary.h
    Core/Json.h

    Database/ReflectionDatabase.h
    Database/ReflectionDatabase.cpp
    Database/DataTypes.h
    Database/DataTypes.cpp
    Database/ReflectionProjectTypes.h

    CodeGenerators/CodeGenerator_CPP_Enum.h
    CodeGenerators/CodeGenerator_CPP_Enum.cpp
    CodeGenerators/CodeGenerator_CPP_Type.h
    CodeGenerators/CodeGenerator_CPP_Type.cpp
    CodeGenerators/CodeGenerator_CPP_Meta.h
    CodeGenerators/CodeGenerator_CPP_Meta.cpp
    CodeGenerators/CodeGenerator_CPP.h
    CodeGenerators/CodeGenerator_CPP.cpp
    CodeGenerators/CodeGenerator_Utils.h
    CodeGenerators/CodeGenerator_Utils.cpp

    CodeGenerators/CodeGenerator_BindingsDataTypes.h
    CodeGenerators/CodeGenerator_BindingsModel.h
    CodeGenerators/CodeGenerator_BindingsModel.cpp
    CodeGenerators/CodeGenerator_BindingsNativeStubs.h
    CodeGenerators/CodeGenerator_BindingsNativeStubs.cpp
    CodeGenerators/CodeGenerator_BindingsTypeMap.h
    CodeGenerators/CodeGenerator_BindingsTypeMap.cpp
    CodeGenerators/CodeGenerator_BindingsCpp.h
    CodeGenerators/CodeGenerator_BindingsCpp.cpp
    CodeGenerators/CodeGenerator_BindingsCSharp.h
    CodeGenerators/CodeGenerator_BindingsCSharp.cpp

    Clang/ClangParser.h
    Clang/ClangParser.cpp
    Clang/ClangParserContext.h
    Clang/ClangParserContext.cpp
    Clang/ClangUtils.h
    Clang/ClangUtils.cpp
    Clang/ClangVisitors_Enum.h
    Clang/ClangVisitors_Enum.cpp
    Clang/ClangVisitors_Macro.h
    Clang/ClangVisitors_Macro.cpp
    Clang/ClangVisitors_Structure.h
    Clang/ClangVisitors_Structure.cpp
    Clang/ClangVisitors_TranslationUnit.h
    Clang/ClangVisitors_TranslationUnit.cpp

    mustache.hpp
)


source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SOURCES})


set(LLVM_INCLUDE_DIRS ${EngineLibDir}/${LLVM}/include)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_CURRENT_SOURCE_DIR}/../Precompile)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_CURRENT_SOURCE_DIR}/../Precompile)
# add LLVM includes
include_directories(${LLVM_INCLUDE_DIRS})

# header files are superflous, but some IDEs (Visual Studio) don't include
# them in the solution explorer without them being added to the list of sources
add_executable(${REFLECTOR_TARGET} ${SOURCES})

target_link_libraries(${REFLECTOR_TARGET} sqlite3 fmt::fmt)

target_include_directories(${REFLECTOR_TARGET}
    PUBLIC ${EngineRootDir}/BuildTool/Code
    PUBLIC ${EngineLibDir}/sqlite3
    PUBLIC ${EngineLibDir}/assimp/contrib/rapidjson/include
    PUBLIC ${EngineLibDir}/fmt/include
)

set_target_properties(${REFLECTOR_TARGET} PROPERTIES CXX_STANDARD 17)
set_target_properties(${REFLECTOR_TARGET} PROPERTIES FOLDER "BuildTool")

if (CMAKE_HOST_WIN32)

    if (CMAKE_BUILD_TYPE STREQUAL Debug)
        set(LLVM_LIBRARY_DIR  ${EngineLibDir}/${LLVM}/lib/DebugLib)
        set(LLVM_SHARED_LIBRARY_DIR  ${EngineLibDir}/${LLVM}/bin/DebugDLL)
    else()
        set(LLVM_LIBRARY_DIR  ${EngineLibDir}/${LLVM}/lib/ReleaseLib)
        set(LLVM_SHARED_LIBRARY_DIR  ${EngineLibDir}/${LLVM}/bin/ReleaseDLL)
    endif()

    # set(LLVM_LIBRARY_DIR  ${EngineLibDir}/LLVM/lib/)
    # set(LLVM_SHARED_LIBRARY_DIR  ${EngineLibDir}/LLVM/bin/)

    # set(CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS} /O2 /Ob2")

    target_link_libraries(${REFLECTOR_TARGET}
            LLVMCore
            LLVMSupport
            LLVMBinaryFormat
            LLVMRemarks
            LLVMFrontendOpenMP
            LLVMBitstreamReader
            clangAST
            clangBasic
            clangLex
            libclang
        )

elseif(${CMAKE_HOST_SYSTEM_NAME} STREQUAL "Linux")
    set(LLVM_LIBRARY_DIR  ${EngineLibDir}/${LLVM}/lib/Linux)
    set(LLVM_SHARED_LIBRARY_DIR  ${EngineLibDir}/${LLVM}/bin/Linux)
    set(CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS} -O3")
    target_link_libraries(${REFLECTOR_TARGET} ${LLVM_SHARED_LIBRARY_DIR}/libclang.so.12)

elseif(${CMAKE_HOST_SYSTEM_PROCESSOR} STREQUAL "arm64") 
    # link to XCode Toolchains' universal binary libclang.dylib 
    set(LLVM_LIBRARY_DIR  ${EngineLibDir}/${LLVM}/lib/macOS)
    set(LLVM_SHARED_LIBRARY_DIR  ${EngineLibDir}/Toolchains/XcodeDefault.xctoolchain/usr/lib)
    set(CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS} -O3")
    target_link_libraries(${REFLECTOR_TARGET} ${EngineLibDir}/libclang.dylib)    
else()
    set(LLVM_LIBRARY_DIR  ${EngineLibDir}/${LLVM}/lib/macOS)
    set(LLVM_SHARED_LIBRARY_DIR  ${EngineLibDir}/${LLVM}/bin/macOS)
    set(CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS} -O3")
    target_link_libraries(${REFLECTOR_TARGET} ${EngineLibDir}/libclang.dylib)
endif()

target_link_directories(${REFLECTOR_TARGET} PUBLIC ${LLVM_LIBRARY_DIR})

file(GLOB LLVM_RUNTIME_FILES CONFIGURE_DEPENDS "${LLVM_SHARED_LIBRARY_DIR}/*")

# copy resources on post build
add_custom_command(TARGET ${REFLECTOR_TARGET} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory
        $<TARGET_FILE_DIR:${REFLECTOR_TARGET}>
	COMMAND ${CMAKE_COMMAND} -E copy_if_different
		${LLVM_RUNTIME_FILES}
		$<TARGET_FILE_DIR:${REFLECTOR_TARGET}>
)
