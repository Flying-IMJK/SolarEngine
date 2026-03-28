#----------------------------------------------------------------
# Generated CMake target import file for configuration "Release".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "GameNetworkingSockets::GameNetworkingSockets" for configuration "Release"
set_property(TARGET GameNetworkingSockets::GameNetworkingSockets APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(GameNetworkingSockets::GameNetworkingSockets PROPERTIES
  IMPORTED_IMPLIB_RELEASE "${_IMPORT_PREFIX}/lib/GameNetworkingSockets.lib"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/bin/GameNetworkingSockets.dll"
  )

list(APPEND _IMPORT_CHECK_TARGETS GameNetworkingSockets::GameNetworkingSockets )
list(APPEND _IMPORT_CHECK_FILES_FOR_GameNetworkingSockets::GameNetworkingSockets "${_IMPORT_PREFIX}/lib/GameNetworkingSockets.lib" "${_IMPORT_PREFIX}/bin/GameNetworkingSockets.dll" )

# Import target "GameNetworkingSockets::GameNetworkingSockets_s" for configuration "Release"
set_property(TARGET GameNetworkingSockets::GameNetworkingSockets_s APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(GameNetworkingSockets::GameNetworkingSockets_s PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_RELEASE "C;CXX"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/GameNetworkingSockets_s.lib"
  )

list(APPEND _IMPORT_CHECK_TARGETS GameNetworkingSockets::GameNetworkingSockets_s )
list(APPEND _IMPORT_CHECK_FILES_FOR_GameNetworkingSockets::GameNetworkingSockets_s "${_IMPORT_PREFIX}/lib/GameNetworkingSockets_s.lib" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
