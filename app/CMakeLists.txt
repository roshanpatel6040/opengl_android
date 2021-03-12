cmake_minimum_required(VERSION 3.4.1)

add_library(opengl
        SHARED
        src/main/cpp/opengl.cpp)

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


include_directories(src/main/cpp/stb_image)
include_directories(src/main/cpp/glm)
include_directories(src/main/cpp/core/headers)

find_library(log-lib log)
find_library(camera-lib camera2ndk)
find_library(media-lib mediandk)

target_link_libraries( # Specifies the target library.
        cameraCpp
        renderer
        vertexBuffer
        indexBuffer
        shader
        texture
        android
        cameraView
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
        )
