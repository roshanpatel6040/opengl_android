#----------------------------------------------------------------
# Generated CMake target import file for configuration "Release".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "assimp::android_jniiosystem" for configuration "Release"
set_property(TARGET assimp::android_jniiosystem APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(assimp::android_jniiosystem PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_RELEASE "CXX"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libandroid_jniiosystem.a"
  )

list(APPEND _IMPORT_CHECK_TARGETS assimp::android_jniiosystem )
list(APPEND _IMPORT_CHECK_FILES_FOR_assimp::android_jniiosystem "${_IMPORT_PREFIX}/lib/libandroid_jniiosystem.a" )

# Import target "assimp::assimp" for configuration "Release"
set_property(TARGET assimp::assimp APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(assimp::assimp PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libassimp.so"
  IMPORTED_SONAME_RELEASE "libassimp.so"
  )

list(APPEND _IMPORT_CHECK_TARGETS assimp::assimp )
list(APPEND _IMPORT_CHECK_FILES_FOR_assimp::assimp "${_IMPORT_PREFIX}/lib/libassimp.so" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
