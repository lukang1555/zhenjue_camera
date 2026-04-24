#----------------------------------------------------------------
# Generated CMake target import file.
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "arm_camera_package::arm_camera_core" for configuration ""
set_property(TARGET arm_camera_package::arm_camera_core APPEND PROPERTY IMPORTED_CONFIGURATIONS NOCONFIG)
set_target_properties(arm_camera_package::arm_camera_core PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_NOCONFIG "CXX"
  IMPORTED_LOCATION_NOCONFIG "${_IMPORT_PREFIX}/lib/libarm_camera_core.a"
  )

list(APPEND _IMPORT_CHECK_TARGETS arm_camera_package::arm_camera_core )
list(APPEND _IMPORT_CHECK_FILES_FOR_arm_camera_package::arm_camera_core "${_IMPORT_PREFIX}/lib/libarm_camera_core.a" )

# Import target "arm_camera_package::arm_camera_node" for configuration ""
set_property(TARGET arm_camera_package::arm_camera_node APPEND PROPERTY IMPORTED_CONFIGURATIONS NOCONFIG)
set_target_properties(arm_camera_package::arm_camera_node PROPERTIES
  IMPORTED_LOCATION_NOCONFIG "${_IMPORT_PREFIX}/lib/arm_camera_package/arm_camera_node"
  )

list(APPEND _IMPORT_CHECK_TARGETS arm_camera_package::arm_camera_node )
list(APPEND _IMPORT_CHECK_FILES_FOR_arm_camera_package::arm_camera_node "${_IMPORT_PREFIX}/lib/arm_camera_package/arm_camera_node" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
