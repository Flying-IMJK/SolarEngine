# 
function(GetAllTargets var startDirectory ignoreDirectorys)
    set(targets)
    GetAllTargetsRecursive(targets ${startDirectory} ignoreDirectorys)
    set(${var} ${targets} PARENT_SCOPE)
endfunction()

macro(GetAllTargetsRecursive targets dir ignoreDirectorys)
    get_property(subdirectories DIRECTORY ${dir} PROPERTY SUBDIRECTORIES)

    set(findIndex)
    foreach(subdir ${subdirectories})
        if(subdir IN_LIST ignoreDirectorys)
            
        else()
            GetAllTargetsRecursive(${targets} ${subdir} ${ignoreDirectorys})
        endif()
    endforeach()

    get_property(current_targets DIRECTORY ${dir} PROPERTY BUILDSYSTEM_TARGETS)
    list(APPEND ${targets} ${current_targets})
endmacro()

set(PrecompileToolsPath "${CMAKE_CURRENT_SOURCE_DIR}/BuildTool/Precompile")
set(SGEPreCompileParamsPath "${PrecompileToolsPath}/precompilefile.json")

set(ignoreDirectorys
    ${EngineRootDir}/Reflector/Code
    ${EngineRootDir}/Libraries
    ${EngineDir}/Assets
)

# 获取所有Target
GetAllTargets(all_targets .. "${ignoreDirectorys}")

###### 写入配置文件
set(all_targets_config "")

foreach(target ${all_targets})
  get_target_property(TargetNameProperty ${target} PrecompileTargetName)
  get_target_property(TargetDirProperty ${target} TargetDir)
  get_target_property(TargetIncludeProperty ${target} TargetInclude)

  if (${TargetNameProperty} STREQUAL TargetNameProperty-NOTFOUND)
    continue()
  endif ()

    # 将当前目标的配置数据添加到累积变量中
  set(single_target_config "{\n    \"TargetName\" : \"${TargetNameProperty}\",\n    \"TargetDir\" : \"${TargetDirProperty}\",\n    \"TargetInclude\" : \"${TargetIncludeProperty}\"\n},\n")

  string(APPEND all_targets_config "${single_target_config}")
endforeach()

string(REGEX REPLACE ",\n$" "" all_targets_config "${all_targets_config}")
set(all_targets_json "[\n${all_targets_config}\n]")
file(WRITE "${SGEPreCompileParamsPath}" "${all_targets_json}")

#
# use wine for linux
if (CMAKE_HOST_WIN32)
    set(PrecompilePreExe)
	  set(PrecompileParser ${PrecompileToolsPath}/SEBuildTool.exe)
    set(sys_include "*") 
elseif(${CMAKE_HOST_SYSTEM_NAME} STREQUAL "Linux" )
    set(PrecompilePreExe)
	  set(PrecompileParser ${PrecompileToolsPath}/SEBuildTool)
    set(sys_include "/usr/include/c++/9/") 
    #execute_process(COMMAND chmod a+x ${PrecompileParser} WORKING_DIRECTORY ${PrecompileToolsPath})
elseif(CMAKE_HOST_APPLE)
    find_program(XCRUN_EXECUTABLE xcrun)
    if(NOT XCRUN_EXECUTABLE)
      message(FATAL_ERROR "xcrun not found!!!")
    endif()

    execute_process(
      COMMAND ${XCRUN_EXECUTABLE} --sdk macosx --show-sdk-platform-path
      OUTPUT_VARIABLE osx_sdk_platform_path_test
      OUTPUT_STRIP_TRAILING_WHITESPACE
    )

    set(PrecompilePreExe)
	  set(PrecompileParser ${PrecompileToolsPath}/SEBuildTool)
    set(sys_include "${osx_sdk_platform_path_test}/../../Toolchains/XcodeDefault.xctoolchain/usr/include/c++/v1") 
endif()


set(ParserInput ${CMAKE_BINARY_DIR}/parser_header.h)
### BUILDING ====================================================================================
set(PRECOMPILE_REBUILD_TARGET "PreCompileRebuild")
set(PRECOMPILE_CLEAR_TARGET "PreCompileClear")
set(PRECOMPILE_TARGET "PreCompileUpdate")


# Called first time when building target 

### Rebuild ====================================================================================
add_custom_target(${PRECOMPILE_REBUILD_TARGET}
COMMAND ${PrecompileParser} -r ${EngineRootDir} -s "${SGEPreCompileParamsPath}" -rebuild
COMMAND ${CMAKE_COMMAND} -E echo "+++ Precompile Rebuild finished +++"
)

### Clear ====================================================================================
add_custom_target(${PRECOMPILE_CLEAR_TARGET}
COMMAND ${PrecompileParser} -r ${EngineRootDir} -s "${SGEPreCompileParamsPath}" -clean
COMMAND ${CMAKE_COMMAND} -E echo "+++ Precompile Clear finished +++"
)


### Update ====================================================================================
add_custom_target(${PRECOMPILE_TARGET}
COMMAND ${PrecompileParser} -r ${EngineRootDir} -s "${SGEPreCompileParamsPath}"
COMMAND ${CMAKE_COMMAND} -E echo "+++ Precompile Update finished +++"
)

set_target_properties(${PRECOMPILE_REBUILD_TARGET} PROPERTIES FOLDER "Src" )
set_target_properties(${PRECOMPILE_CLEAR_TARGET} PROPERTIES FOLDER "Src" )
set_target_properties(${PRECOMPILE_TARGET} PROPERTIES FOLDER "Src" )
