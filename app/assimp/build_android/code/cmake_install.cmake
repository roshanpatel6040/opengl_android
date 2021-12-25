# Install script for directory: /Users/roshan/cProjects/assimp-5.1.3/code

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "/usr/local")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "Release")
  endif()
  message(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
endif()

# Set the component getting installed.
if(NOT CMAKE_INSTALL_COMPONENT)
  if(COMPONENT)
    message(STATUS "Install component: \"${COMPONENT}\"")
    set(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  else()
    set(CMAKE_INSTALL_COMPONENT)
  endif()
endif()

# Install shared libraries without execute permission?
if(NOT DEFINED CMAKE_INSTALL_SO_NO_EXE)
  set(CMAKE_INSTALL_SO_NO_EXE "0")
endif()

# Is this installation the result of a crosscompile?
if(NOT DEFINED CMAKE_CROSSCOMPILING)
  set(CMAKE_CROSSCOMPILING "TRUE")
endif()

# Set default install directory permissions.
if(NOT DEFINED CMAKE_OBJDUMP)
  set(CMAKE_OBJDUMP "/Users/roshan/Library/Android/sdk/ndk/23.1.7779620/toolchains/llvm/prebuilt/darwin-x86_64/bin/llvm-objdump")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libassimp.so" AND
     NOT IS_SYMLINK "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libassimp.so")
    file(RPATH_CHECK
         FILE "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libassimp.so"
         RPATH "")
  endif()
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE SHARED_LIBRARY FILES "/Users/roshan/cProjects/assimp-5.1.3/build_android/bin/libassimp.so")
  if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libassimp.so" AND
     NOT IS_SYMLINK "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libassimp.so")
    if(CMAKE_INSTALL_DO_STRIP)
      execute_process(COMMAND "/Users/roshan/Library/Android/sdk/ndk/23.1.7779620/toolchains/llvm/prebuilt/darwin-x86_64/bin/llvm-strip" "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libassimp.so")
    endif()
  endif()
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xassimp-devx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/assimp" TYPE FILE FILES
    "/Users/roshan/cProjects/assimp-5.1.3/code/../include/assimp/anim.h"
    "/Users/roshan/cProjects/assimp-5.1.3/code/../include/assimp/aabb.h"
    "/Users/roshan/cProjects/assimp-5.1.3/code/../include/assimp/ai_assert.h"
    "/Users/roshan/cProjects/assimp-5.1.3/code/../include/assimp/camera.h"
    "/Users/roshan/cProjects/assimp-5.1.3/code/../include/assimp/color4.h"
    "/Users/roshan/cProjects/assimp-5.1.3/code/../include/assimp/color4.inl"
    "/Users/roshan/cProjects/assimp-5.1.3/build_android/code/../include/assimp/config.h"
    "/Users/roshan/cProjects/assimp-5.1.3/code/../include/assimp/ColladaMetaData.h"
    "/Users/roshan/cProjects/assimp-5.1.3/code/../include/assimp/commonMetaData.h"
    "/Users/roshan/cProjects/assimp-5.1.3/code/../include/assimp/defs.h"
    "/Users/roshan/cProjects/assimp-5.1.3/code/../include/assimp/cfileio.h"
    "/Users/roshan/cProjects/assimp-5.1.3/code/../include/assimp/light.h"
    "/Users/roshan/cProjects/assimp-5.1.3/code/../include/assimp/material.h"
    "/Users/roshan/cProjects/assimp-5.1.3/code/../include/assimp/material.inl"
    "/Users/roshan/cProjects/assimp-5.1.3/code/../include/assimp/matrix3x3.h"
    "/Users/roshan/cProjects/assimp-5.1.3/code/../include/assimp/matrix3x3.inl"
    "/Users/roshan/cProjects/assimp-5.1.3/code/../include/assimp/matrix4x4.h"
    "/Users/roshan/cProjects/assimp-5.1.3/code/../include/assimp/matrix4x4.inl"
    "/Users/roshan/cProjects/assimp-5.1.3/code/../include/assimp/mesh.h"
    "/Users/roshan/cProjects/assimp-5.1.3/code/../include/assimp/pbrmaterial.h"
    "/Users/roshan/cProjects/assimp-5.1.3/code/../include/assimp/GltfMaterial.h"
    "/Users/roshan/cProjects/assimp-5.1.3/code/../include/assimp/postprocess.h"
    "/Users/roshan/cProjects/assimp-5.1.3/code/../include/assimp/quaternion.h"
    "/Users/roshan/cProjects/assimp-5.1.3/code/../include/assimp/quaternion.inl"
    "/Users/roshan/cProjects/assimp-5.1.3/code/../include/assimp/scene.h"
    "/Users/roshan/cProjects/assimp-5.1.3/code/../include/assimp/metadata.h"
    "/Users/roshan/cProjects/assimp-5.1.3/code/../include/assimp/texture.h"
    "/Users/roshan/cProjects/assimp-5.1.3/code/../include/assimp/types.h"
    "/Users/roshan/cProjects/assimp-5.1.3/code/../include/assimp/vector2.h"
    "/Users/roshan/cProjects/assimp-5.1.3/code/../include/assimp/vector2.inl"
    "/Users/roshan/cProjects/assimp-5.1.3/code/../include/assimp/vector3.h"
    "/Users/roshan/cProjects/assimp-5.1.3/code/../include/assimp/vector3.inl"
    "/Users/roshan/cProjects/assimp-5.1.3/code/../include/assimp/version.h"
    "/Users/roshan/cProjects/assimp-5.1.3/code/../include/assimp/cimport.h"
    "/Users/roshan/cProjects/assimp-5.1.3/code/../include/assimp/importerdesc.h"
    "/Users/roshan/cProjects/assimp-5.1.3/code/../include/assimp/Importer.hpp"
    "/Users/roshan/cProjects/assimp-5.1.3/code/../include/assimp/DefaultLogger.hpp"
    "/Users/roshan/cProjects/assimp-5.1.3/code/../include/assimp/ProgressHandler.hpp"
    "/Users/roshan/cProjects/assimp-5.1.3/code/../include/assimp/IOStream.hpp"
    "/Users/roshan/cProjects/assimp-5.1.3/code/../include/assimp/IOSystem.hpp"
    "/Users/roshan/cProjects/assimp-5.1.3/code/../include/assimp/Logger.hpp"
    "/Users/roshan/cProjects/assimp-5.1.3/code/../include/assimp/LogStream.hpp"
    "/Users/roshan/cProjects/assimp-5.1.3/code/../include/assimp/NullLogger.hpp"
    "/Users/roshan/cProjects/assimp-5.1.3/code/../include/assimp/cexport.h"
    "/Users/roshan/cProjects/assimp-5.1.3/code/../include/assimp/Exporter.hpp"
    "/Users/roshan/cProjects/assimp-5.1.3/code/../include/assimp/DefaultIOStream.h"
    "/Users/roshan/cProjects/assimp-5.1.3/code/../include/assimp/DefaultIOSystem.h"
    "/Users/roshan/cProjects/assimp-5.1.3/code/../include/assimp/ZipArchiveIOSystem.h"
    "/Users/roshan/cProjects/assimp-5.1.3/code/../include/assimp/SceneCombiner.h"
    "/Users/roshan/cProjects/assimp-5.1.3/code/../include/assimp/fast_atof.h"
    "/Users/roshan/cProjects/assimp-5.1.3/code/../include/assimp/qnan.h"
    "/Users/roshan/cProjects/assimp-5.1.3/code/../include/assimp/BaseImporter.h"
    "/Users/roshan/cProjects/assimp-5.1.3/code/../include/assimp/Hash.h"
    "/Users/roshan/cProjects/assimp-5.1.3/code/../include/assimp/MemoryIOWrapper.h"
    "/Users/roshan/cProjects/assimp-5.1.3/code/../include/assimp/ParsingUtils.h"
    "/Users/roshan/cProjects/assimp-5.1.3/code/../include/assimp/StreamReader.h"
    "/Users/roshan/cProjects/assimp-5.1.3/code/../include/assimp/StreamWriter.h"
    "/Users/roshan/cProjects/assimp-5.1.3/code/../include/assimp/StringComparison.h"
    "/Users/roshan/cProjects/assimp-5.1.3/code/../include/assimp/StringUtils.h"
    "/Users/roshan/cProjects/assimp-5.1.3/code/../include/assimp/SGSpatialSort.h"
    "/Users/roshan/cProjects/assimp-5.1.3/code/../include/assimp/GenericProperty.h"
    "/Users/roshan/cProjects/assimp-5.1.3/code/../include/assimp/SpatialSort.h"
    "/Users/roshan/cProjects/assimp-5.1.3/code/../include/assimp/SkeletonMeshBuilder.h"
    "/Users/roshan/cProjects/assimp-5.1.3/code/../include/assimp/SmallVector.h"
    "/Users/roshan/cProjects/assimp-5.1.3/code/../include/assimp/SmoothingGroups.h"
    "/Users/roshan/cProjects/assimp-5.1.3/code/../include/assimp/SmoothingGroups.inl"
    "/Users/roshan/cProjects/assimp-5.1.3/code/../include/assimp/StandardShapes.h"
    "/Users/roshan/cProjects/assimp-5.1.3/code/../include/assimp/RemoveComments.h"
    "/Users/roshan/cProjects/assimp-5.1.3/code/../include/assimp/Subdivision.h"
    "/Users/roshan/cProjects/assimp-5.1.3/code/../include/assimp/Vertex.h"
    "/Users/roshan/cProjects/assimp-5.1.3/code/../include/assimp/LineSplitter.h"
    "/Users/roshan/cProjects/assimp-5.1.3/code/../include/assimp/TinyFormatter.h"
    "/Users/roshan/cProjects/assimp-5.1.3/code/../include/assimp/Profiler.h"
    "/Users/roshan/cProjects/assimp-5.1.3/code/../include/assimp/LogAux.h"
    "/Users/roshan/cProjects/assimp-5.1.3/code/../include/assimp/Bitmap.h"
    "/Users/roshan/cProjects/assimp-5.1.3/code/../include/assimp/XMLTools.h"
    "/Users/roshan/cProjects/assimp-5.1.3/code/../include/assimp/IOStreamBuffer.h"
    "/Users/roshan/cProjects/assimp-5.1.3/code/../include/assimp/CreateAnimMesh.h"
    "/Users/roshan/cProjects/assimp-5.1.3/code/../include/assimp/XmlParser.h"
    "/Users/roshan/cProjects/assimp-5.1.3/code/../include/assimp/BlobIOSystem.h"
    "/Users/roshan/cProjects/assimp-5.1.3/code/../include/assimp/MathFunctions.h"
    "/Users/roshan/cProjects/assimp-5.1.3/code/../include/assimp/Exceptional.h"
    "/Users/roshan/cProjects/assimp-5.1.3/code/../include/assimp/ByteSwapper.h"
    )
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xassimp-devx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/assimp/Compiler" TYPE FILE FILES
    "/Users/roshan/cProjects/assimp-5.1.3/code/../include/assimp/Compiler/pushpack1.h"
    "/Users/roshan/cProjects/assimp-5.1.3/code/../include/assimp/Compiler/poppack1.h"
    "/Users/roshan/cProjects/assimp-5.1.3/code/../include/assimp/Compiler/pstdint.h"
    )
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xassimp-devx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include" TYPE FILE FILES "/Users/roshan/cProjects/assimp-5.1.3/code/../include/assimp/port/AndroidJNI/AndroidJNIIOSystem.h")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for each subdirectory.
  include("/Users/roshan/cProjects/assimp-5.1.3/build_android/port/AndroidJNI/cmake_install.cmake")

endif()

