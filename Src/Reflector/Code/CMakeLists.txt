#cmake_policy(SET CMP0074 OLD)
set(REFLECTOR_TARGET SE.Reflector)

#set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/../CMake")

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

include(../../Libraries/GameNetworkingSockets/link.cmake)
set(LLVM "LLVM15")

set("SOURCES"
    Reflector.h
    Reflector.cpp
    ReflectorSettingsAndUtils.h
    ReflectorSettingsAndUtils.cpp

    Database/ReflectionDatabase.h
    Database/ReflectionDatabase.cpp
    Database/ReflectionDataTypes.h
    Database/ReflectionDataTypes.cpp
    Database/ReflectionProjectTypes.h

    CodeGenerators/CodeGenerator_CPP_Enum.h
    CodeGenerators/CodeGenerator_CPP_Enum.cpp
    CodeGenerators/CodeGenerator_CPP_Type.h
    CodeGenerators/CodeGenerator_CPP_Type.cpp
    CodeGenerators/CodeGenerator_CPP_Meta.h
    CodeGenerators/CodeGenerator_CPP_Meta.cpp
    CodeGenerators/CodeGenerator_CPP.h
    CodeGenerators/CodeGenerator_CPP.cpp

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

    TopologicalSort.h
    TopologicalSort.cpp
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

target_link_libraries(${REFLECTOR_TARGET} SE.Core)

target_include_directories(${REFLECTOR_TARGET}
    PUBLIC ${EngineRootDir}/
)

set_target_properties(${REFLECTOR_TARGET} PROPERTIES CXX_STANDARD 17)
set_target_properties(${REFLECTOR_TARGET} PROPERTIES FOLDER "Reflector")

if (CMAKE_HOST_WIN32)

    if (CMAKE_BUILD_TYPE STREQUAL Debug)
        set(LLVM_LIBRARY_DIR  ${EngineLibDir}/${LLVM}/lib/debug/)
        set(LLVM_SHARED_LIBRARY_DIR  ${EngineLibDir}/${LLVM}/bin/debug/)
    else()
        set(LLVM_LIBRARY_DIR  ${EngineLibDir}/${LLVM}/lib/release)
        set(LLVM_SHARED_LIBRARY_DIR  ${EngineLibDir}/${LLVM}/bin/release)
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

GameNetworkingSocketsLinkReflector(${REFLECTOR_TARGET})

# copy resources on post build
add_custom_command(TARGET ${REFLECTOR_TARGET} POST_BUILD
	# mustache templates directory
	COMMAND ${CMAKE_COMMAND} -E copy_directory
		"${LLVM_SHARED_LIBRARY_DIR}"
		$<TARGET_FILE_DIR:${REFLECTOR_TARGET}>
    COMMAND ${CMAKE_COMMAND} -E copy
		"${ProjectBuildDir}/SE.Core.dll" "${ProjectBuildDir}/SE.Core.pdb"
		$<TARGET_FILE_DIR:${REFLECTOR_TARGET}>
)
