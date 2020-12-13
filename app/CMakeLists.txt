cmake_minimum_required(VERSION 3.4.1)

add_library(opengl
        SHARED
        src/main/cpp/opengl.cpp)

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

include_directories(src/main/cpp/stb_image)
include_directories(src/main/cpp/glm)
include_directories(src/main/cpp/core/headers)

find_library(log-lib
        log)

target_link_libraries( # Specifies the target library.
        opengl
        renderer
        vertexBuffer
        indexBuffer
        shader
        texture
        android
        ${log-lib}
        EGL
        GLESv2
        )
