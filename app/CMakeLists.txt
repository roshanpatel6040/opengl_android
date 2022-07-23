cmake_minimum_required(VERSION 3.4.1)

add_library(opengl
        SHARED
        src/main/cpp/opengl.cpp)

add_library(arcoreImpl SHARED src/main/cpp/arcoreimpl.cpp)

add_library(cameraCpp
        SHARED
        src/main/cpp/camera.cpp)

add_library(renderer
        STATIC
        src/main/cpp/core/Renderer.cpp)

add_library(vertexBuffer
        STATIC
        src/main/cpp/core/VextexBuffer.cpp)

add_library(indexBuffer
        STATIC
        src/main/cpp/core/IndexBuffer.cpp)

add_library(shader
        STATIC
        src/main/cpp/core/Shader.cpp)

add_library(texture
        STATIC
        src/main/cpp/core/Texture.cpp)

add_library(cameraView
        STATIC
        src/main/cpp/core/CameraView.cpp)

add_library(mesh
        STATIC
        src/main/cpp/core/Mesh.cpp)

add_library(worldTransform
        STATIC
        src/main/cpp/core/WorldTransform.cpp)

add_library(math
        STATIC
        src/main/cpp/core/MathHelper.cpp)

add_library(ar STATIC src/main/cpp/core/ar.cpp)

add_library(depthTexture STATIC src/main/cpp/core/DepthTexture.cpp)

add_library(renderers STATIC
        src/main/cpp/core/background_renderer.cpp
        src/main/cpp/core/models/BobLampCleanRenderer.cpp
        src/main/cpp/core/models/Circle.cpp
        src/main/cpp/core/plane_renderer.cc
        )

include_directories(src/main/cpp/stb_image)
include_directories(src/main/cpp/glm)
include_directories(src/main/cpp/core/headers)

find_library(log-lib log)
find_library(camera-lib camera2ndk)
find_library(media-lib mediandk)
#find_library(shared-lib libc++_shared.so)

set(ENABLE_BOOST_WORKAROUND ON)
set(BUILD_STATIC_LIB ON)
set(BUILD_ASSIMP_TOOLS  ON)
set(ASSIMP_BUILD_STATIC_LIB ON)

add_subdirectory(assimp)
include_directories(assimp/include)
include_directories(assimp/build_android/include)
set(ASSIMP_LIB assimp)

#add_library(JNIO STATIC IMPORTED)
#set_target_properties(JNIO PROPERTIES IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/assimp/build_android/port/AndroidJNI/libandroid_jniiosystem.a)

add_library(arcore SHARED IMPORTED)
set_target_properties(arcore PROPERTIES IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/src/main/jni/${ANDROID_ABI}/libarcore_sdk_c.so
        INTERFACE_INCLUDE_DIRECTORIES ${CMAKE_CURRENT_SOURCE_DIR}/arcore/include)
include_directories(src/main/arcore/include)

target_link_libraries( # Specifies the target library.
        arcoreImpl
        renderer
        vertexBuffer
        indexBuffer
        shader
        texture
        android
        cameraView
        mesh
        worldTransform
        math
        arcore
        ar
        depthTexture
        renderers
        ${ASSIMP_LIB}
        ${log-lib}
        EGL
        GLESv2
        GLESv3
        )

target_link_libraries( # Specifies the target library.
        cameraCpp
        renderer
        vertexBuffer
        indexBuffer
        shader
        texture
        android
        cameraView
        mesh
        worldTransform
        math
        renderers
        ${ASSIMP_LIB}
        ${log-lib}
        ${camera-lib}
        ${media-lib}
        EGL
        GLESv2
        GLESv3
        )

target_link_libraries( # Specifies the target library.
        opengl
        renderer
        vertexBuffer
        indexBuffer
        shader
        texture
        cameraView
        android
        ${log-lib}
        EGL
        GLESv2
        GLESv3
        ${ASSIMP_LIB}
        )
