//
// Created by Roshan
//

#include <cstdio>
#include <jni.h>
#include <EGL/egl.h>
#include <GLES/gl.h>
#include <GLES/glplatform.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2platform.h>
#include <string>
#include <android/log.h>
#include <iostream>
#include <cassert>
#include "stb_image/stb_image.cpp"
#include "glm/glm.hpp"
#include "glm/ext.hpp"
#include <android/asset_manager_jni.h>
#include <android/asset_manager.h>
#include "core/headers/Renderer.h"
#include "core/headers/Shader.h"
#include "core/headers/Texture.h"
#include "core/headers/VertexBuffer.h"

using namespace std;

int windowWidth = 0;
int windowHeight = 0;
AAssetManager *assets;

void readObj(const char *path) {
    AAsset *file = AAssetManager_open(assets, path, AASSET_MODE_UNKNOWN);
    long length = AAsset_getLength(file);
    char *buffer = (char *) malloc(sizeof(char) * length);
    AAsset_read(file, buffer, length);
    __android_log_print(ANDROID_LOG_DEBUG, "Reading model content", "%s", buffer);
    AAsset_close(file);
    free(buffer);
}

int loadShader(GLenum type, const std::string &shaderCode) {
    GLuint shader = glCreateShader(type);
    const char *source = shaderCode.c_str();
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);
    if (shader == -1) {
        __android_log_print(ANDROID_LOG_ERROR, "Shader", "%d", shader);
    }
    return shader;
}

void drawPyramid() {
    float vertices[] = {
            0.0f, 1.0f, 0.0f,
            -1.0f, 0.0f, -1.0f,
            1.0f, 0.0f, -1.0f,
            1.0f, 0.0f, 1.0f,
            -1.0f, 0.0f, 1.0f,
    };
    // Text chords before texture at 4 points(direction)
    // values are fixed 0.0f to 1.0f
    float texChords[] = {
            0.0f, 1.0f, // to map texture bottom left
            0.0f, 0.0f, // to map texture bottom right
            1.0f, 0.0f,// to map texture top right
            1.0f, 1.0f,// to map texture top left
    };

    // order floats for drawing triangles
    // Re use number for anti clockwise
    // This order will create two triangles which makes it square
//    unsigned int order[] = {1, 2, 3, 1, 3, 4, 1, 4, 5, 1, 5, 2, 3, 2, 4, 2, 5, 4};
    unsigned int order[] = {0, 4, 3, 0, 3, 2, 0, 2, 1, 0, 1, 4, 4, 2, 1, 4, 3, 2};
    // RGBA color format
    float color[] = {0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, 1.0f};
    int CHORDS_PER_VERTEX = 3;

    // Works from bottom
    glm::mat4 ortho = glm::ortho(-1.0f, (float) 1.0, -1.0f, (float) 1.0, -1.0f,
                                 100.0f);
    glm::mat4 perspective = glm::perspective(glm::radians(
            90.0f), // The vertical Field of View, in radians: the amount of "zoom". Think "camera lens". Usually between 90° (extra wide) and 30° (quite zoomed in)
                                             (float) windowWidth /
                                             (float) windowHeight,       // Aspect Ratio. Depends on the size of your window. Notice that 4/3 == 800/600 == 1280/960, sounds familiar ?
                                             0.1f,              // Near clipping plane. Keep as big as possible, or you'll get precision issues.
                                             100.0f             // Far clipping plane. Keep as little as possible.
    );
    // Translation
    glm::mat4 translation = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 0.0f));
    // Scaling
    glm::mat4 scaleMatrix = glm::scale(glm::mat4(1.0f), glm::vec3(3.0f));
    // Rotation
    glm::mat4 rotationMatrixX = rotate(glm::mat4(1.0f), glm::radians(5.0f),
                                       glm::vec3(1.0f, 0.0f, 0.0f));
    glm::mat4 rotationMatrixY = rotate(glm::mat4(1.0f), glm::radians(45.0f),
                                       glm::vec3(0.0f, 1.0f, 0.0f));
    // Camera matrix
    glm::mat4 cameraMatrix = glm::lookAt(
            glm::vec3(0.0f, 0.0f, 3.0f), // the position of your camera, in world space
            glm::vec3(0.0f, 0.0f, 0.0f), // where you want to look at, in world space
            glm::vec3(0.0f, 1.0f,
                      0.0f) // probably glm::vec3(0,1,0), but (0,-1,0) would make you looking upside-down, which can be great too
    );
    // Model matrix
    glm::mat4 modelMatrix = translation * scaleMatrix;
    // final projection
    glm::mat4 projection = perspective;

    string vertexTextureShader = "attribute vec3 position;\n"
                                 //                                 "attribute vec2 texChords;\n"
                                 "uniform mat4 u_MVP;\n"
                                 //                                 "varying vec2 chords;\n"
                                 "void main()\n"
                                 "{\n"
                                 " gl_Position = vec4(position,1.0);\n"
                                 //                                 " chords = texChords;\n"
                                 "}";

    string fragmentTextureShader = "uniform vec4 color[2];\n" // Passing multiple rgba
                                   //                                   "uniform sampler2D u_texture;\n"
                                   //                                   "varying vec2 chords;\n"
                                   "void main()\n"
                                   "{\n"
                                   //                                   " vec4 textureColor = texture2D(u_texture,chords);\n"
                                   //                                   " gl_FragColor = textureColor;\n" // assign it to texture color
                                   " gl_FragColor = color[0];\n" // select which color is selected from array
                                   "}";

    // Enable blending
    GLCall(glEnable(GL_DEPTH_TEST))
    GLCall(glCullFace(GL_BACK))
    GLCall(glEnable(GL_BLEND))
    GLCall(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA))

    GLCall(GLuint program = glCreateProgram())
    GLCall(glAttachShader(program, loadShader(GL_VERTEX_SHADER, vertexTextureShader)))
    GLCall(glAttachShader(program, loadShader(GL_FRAGMENT_SHADER, fragmentTextureShader)))
    GLCall(glLinkProgram(program))
    GLCall(glUseProgram(program))

    GLCall(GLint positionHandle = glGetAttribLocation(program, "position"))
    GLCall(glEnableVertexAttribArray(positionHandle))
    GLCall(glVertexAttribPointer(positionHandle, 3 /* co-ordinates per vertex */, GL_FLOAT, false,
                                 0, vertices))

    GLCall(GLint colorHandle = glGetUniformLocation(program, "color"))
    GLCall(glUniform4fv(colorHandle, 2, color)) // Passing two rgba

    GLCall(glDrawElements(GL_TRIANGLES, 18, GL_UNSIGNED_INT, order))

//    GLCall(GLint texChordsHandle = glGetAttribLocation(program, "texChords"))
//    GLCall(glEnableVertexAttribArray(texChordsHandle))
//    GLCall(glVertexAttribPointer(texChordsHandle, 3 /* co-ordinates per vertex */, GL_FLOAT, false,0, texChords))

//    GLCall(GLint projectionHandle = glGetUniformLocation(program, "u_MVP"))
//    GLCall(glUniformMatrix4fv(projectionHandle, 1 /* No. of matrix array in our case only 1*/,
//                              false,
//                              glm::value_ptr(
//                                      projection) /* pass reference at 0 0 position other work will be handled by opengl itself Basically opengl will take other values automatically */))

    // Texture code and its uniform location
    // Active texture slot is 0
    // "/storage/emulated/0/Pictures/UHD Wallpapers/_uhdminimal34.jpg"
//    GLuint mReferenceID = drawTextures(
//            "/storage/emulated/0/Pictures/UHD Wallpapers/_uhdminimal34.jpg", 0);
//    GLCall(GLint texturePosition = glGetUniformLocation(program, "u_texture"))
//    GLCall(glUniform1i(texturePosition, 0)) // slot passed as 0

    GLCall(glDisableVertexAttribArray(positionHandle))
    // delete texture
//    GLCall(glDeleteTextures(GL_TEXTURE_2D, &mReferenceID))
}

/**
 * Draw triangle
 */
void drawTriangle() {
    int CHORDS_COLOR_PER_VERTEX = 4;
    int BYTES_PER_FLOAT = 4;
    float vertices[] = {
        //    X    Y     Z     R     G      B    A
            0.0f, 0.5f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f,
            -0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f,
            0.5f, -0.5f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f};

    float color[] = {1.0f, 0.5f, 0.0f, 1.0f};

    // varying field uses to transfer data between vertex and fragment shader
    std::string vertexShaderCode = "attribute vec4 position;\n"
                                   "attribute vec4 v_color;\n"
                                   "varying vec4 f_color;\n" // Passes to fragment shader
                                   "void main()\n"
                                   "{\n"
                                   " f_color = v_color;\n"
                                   " gl_Position = position;\n"
                                   "}";
    std::string fragmentShaderCode = "uniform vec4 color;\n" // Get using uniform location
                                     "varying vec4 f_color;\n" // Get from vertex shader
                                     "void main()\n"
                                     "{\n"
                                     " gl_FragColor = f_color;\n"
                                     //                                " gl_FragColor = color;\n"
                                     "}";

    GLCall(GLuint program = glCreateProgram())
    GLCall(Shader shader(program, vertexShaderCode, fragmentShaderCode))
    GLCall(glLinkProgram(program))
    GLCall(glUseProgram(program))

    GLCall(VertexBuffer vb(vertices, sizeof(vertices)))

    GLCall(GLint positionHandle = shader.getAttributeLocation("position"))
    GLCall(shader.vertexAttribPointer(positionHandle, GL_FLOAT, 3, 7 * sizeof(float),
                                      (GLvoid *) nullptr))
    GLCall(shader.enableVertexAttribArray(positionHandle))

    GLCall(GLuint colorPositionHandle = shader.getAttributeLocation("v_color"))
    GLCall(shader.vertexAttribPointer(colorPositionHandle, GL_FLOAT, 4, 7 * sizeof(float),
                                      (GLvoid *) (3 * sizeof(float))))
    GLCall(shader.enableVertexAttribArray(colorPositionHandle))

    GLCall(int colorHandle = shader.getUniformLocation("color"))
    GLCall(shader.setUniform4fv(colorHandle,
                                sizeof(color) / CHORDS_COLOR_PER_VERTEX / BYTES_PER_FLOAT, color))

    GLCall(glDrawArrays(GL_TRIANGLES, 0, 3))
    GLCall(shader.disableVertexAttribPointer(positionHandle))
    GLCall(shader.disableVertexAttribPointer(colorPositionHandle))
    shader.unBind();
    vb.unBind();
}

void drawSquare() {
    // x, y, z coordinates system
    // Here y = 0 determines bottom left of the screen
    // In android y = 0 determines top left of the screen
    // values are device width and height which are passed static
    // Appropriate orthographic projection works with this points
//    float vertices[] = {
//            0.0f, 2113.0f, 1.0f,
//            0.0f, 0.0f, 1.0f,
//            1080.0f, 0.0f, 1.0f,
//            1080.0f, 2113.0f, 1.0f,
//    };

    // Vertices before orthographical projection or any projection
    // Projection will be counted as -1.0f to 1.0f for left right and bottom top
    // tc1 and tc2 represents texChords for left right and top bottom
    float vertices[] = {
            // x     y     z    tc1   tc2
            -0.5f, 0.5f, 0.0f, 0.0f, 1.0f, // to map texture bottom left
            -0.5f, -0.5f, 0.0f, 0.0f, 0.0f, // to map texture bottom right
            0.5f, -0.5f, 0.0f, 1.0f, 0.0f, // to map texture top right
            0.5f, 0.5f, 0.0f, 1.0f, 1.0f, // to map texture top left
    };

    // Text chords before texture at 4 points(direction)
    // values are fixed 0.0f to 1.0f
    float texChords[] = {
            0.0f, 1.0f, // to map texture bottom left
            0.0f, 0.0f, // to map texture bottom right
            1.0f, 0.0f, // to map texture top right
            1.0f, 1.0f, // to map texture top left
    };

    // order floats for drawing triangles
    // Re use number for anti clockwise
    // This order will create two triangles which makes it square
    unsigned int order[] = {0, 1, 2, 0, 2, 3};

    // RGBA color format
    float color[] = {0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, 1.0f};

    // Works from bottom
    glm::mat4 ortho = glm::ortho(-1.0f, (float) 1.0, -1.0f, (float) 1.0, -1.0f,
                                 100.0f);
    glm::mat4 perspective = glm::perspective(glm::radians(
            90.0f), // The vertical Field of View, in radians: the amount of "zoom". Think "camera lens". Usually between 90° (extra wide) and 30° (quite zoomed in)
                                             (float) windowWidth /
                                             (float) windowHeight,       // Aspect Ratio. Depends on the size of your window. Notice that 4/3 == 800/600 == 1280/960, sounds familiar ?
                                             0.1f,              // Near clipping plane. Keep as big as possible, or you'll get precision issues.
                                             100.0f             // Far clipping plane. Keep as little as possible.
    );
    // Translation
    glm::mat4 translation = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 0.0f));
    // Scaling
    glm::mat4 scaleMatrix = glm::scale(glm::mat4(1.0f), glm::vec3(3.0f));
    // Rotation
    glm::mat4 rotationMatrixX = rotate(glm::mat4(1.0f), glm::radians(5.0f),
                                       glm::vec3(1.0f, 0.0f, 0.0f));
    glm::mat4 rotationMatrixY = rotate(glm::mat4(1.0f), glm::radians(45.0f),
                                       glm::vec3(0.0f, 1.0f, 0.0f));
    // Camera matrix
    glm::mat4 cameraMatrix = glm::lookAt(
            glm::vec3(0.0f, 0.0f, 3.0f), // the position of your camera, in world space
            glm::vec3(0.0f, 0.0f, 0.0f), // where you want to look at, in world space
            glm::vec3(0.0f, 1.0f,
                      0.0f) // probably glm::vec3(0,1,0), but (0,-1,0) would make you looking upside-down, which can be great too
    );
    // Model matrix
    glm::mat4 modelMatrix = translation * scaleMatrix;
    // final projection
    glm::mat4 projection = perspective * cameraMatrix * modelMatrix;

    string vertexTextureShader = "attribute vec3 position;\n"
                                 "attribute vec2 texChords;\n"
                                 "uniform mat4 u_MVP;\n"
                                 "varying vec2 chords;\n"
                                 "void main()\n"
                                 "{\n"
                                 " gl_Position = u_MVP * vec4(position,1.0);\n"
                                 " chords = texChords;\n"
                                 "}";

    string fragmentTextureShader = "uniform vec4 color[2];\n" // Passing multiple rgba
                                   "uniform sampler2D u_texture;\n"
                                   "varying vec2 chords;\n"
                                   "void main()\n"
                                   "{\n"
                                   " vec4 textureColor = texture2D(u_texture,chords);\n"
                                   " gl_FragColor = textureColor;\n" // assign it to texture color
                                   // " gl_FragColor = color[0];\n" // select which color is selected from array
                                   "}";

    // Enable blending
    GLCall(glEnable(GL_BLEND))
    GLCall(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA))

    GLCall(GLuint program = glCreateProgram())
    GLCall(Shader shader(program, vertexTextureShader, fragmentTextureShader))
    GLCall(glLinkProgram(program))

    // Core OpenGL requires that we use a VAO so it knows what to do with our vertex inputs. If we fail to bind a VAO, OpenGL will most likely refuse to draw anything.

    GLCall(VertexBuffer vb(vertices, sizeof(vertices)))
    GLCall(IndexBuffer ib(order, sizeof(order)))

    GLCall(glUseProgram(program))

    GLCall(GLint positionHandle = shader.getAttributeLocation("position"))
    GLCall(shader.vertexAttribPointer(positionHandle, GL_FLOAT, 3, 5 * sizeof(float),
                                      (GLvoid *) nullptr))
    GLCall(shader.enableVertexAttribArray(positionHandle))

    GLCall(GLint texChordsHandle = shader.getAttributeLocation("texChords"))
    GLCall(shader.vertexAttribPointer(texChordsHandle, GL_FLOAT, 2, 5 * sizeof(float),
                                      (GLvoid *) (3 * sizeof(float))))
    GLCall(shader.enableVertexAttribArray(texChordsHandle))

    GLCall(GLint mvpHandle = shader.getUniformLocation("u_MVP"))
    GLCall(shader.setUniformMatrix4fv(mvpHandle, 1 /* No. of matrix array in our case only 1*/,
                                      glm::value_ptr(
                                              projection) /* pass reference at 0 0 position other work will be handled by opengl itself Basically opengl will take other values automatically */))

    GLCall(Texture texture("/storage/emulated/0/Pictures/UHD Wallpapers/_uhdminimal34.jpg", 0))
    GLCall(GLint textureChords = shader.getUniformLocation("u_texture"))
    GLCall(shader.setUniform1i(textureChords, texture.getSlot()))

    GLCall(GLint colorHandle = shader.getUniformLocation("color"))
    GLCall(shader.setUniform4fv(colorHandle, 2, color)) // Passing two rgba in float array

    GLCall(glDrawElements(GL_TRIANGLES, ib.getCount(), GL_UNSIGNED_INT,
                          nullptr))

    GLCall(shader.disableVertexAttribPointer(positionHandle))
    GLCall(shader.disableVertexAttribPointer(texChordsHandle))

    GLCall(texture.~Texture())
    GLCall(shader.unBind())
    GLCall(vb.unBind())
    GLCall(ib.unBind())
}

extern "C" {
void Java_com_demo_opengl_Render_render(JNIEnv *env, jobject object) {
    glClear(GL_COLOR_BUFFER_BIT);
    drawSquare();
    drawTriangle();
//    drawPyramid();
}

void Java_com_demo_opengl_Render_onSurfaceCreated(JNIEnv *env, jobject object) {
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
}

void
Java_com_demo_opengl_Render_onSurfaceChanged(JNIEnv *env, jobject object, jint width,
                                             jint height) {
    glViewport(0, 0, width, height);
    windowWidth = width;
    windowHeight = height;
    __android_log_print(ANDROID_LOG_DEBUG, "onSurfaceChanged", "window:%i height:%i", windowWidth,
                        windowHeight);
}

void
Java_com_demo_opengl_MainActivity_assetsManager(JNIEnv *env, jobject object,
                                                jobject assetsManager) {
    assets = AAssetManager_fromJava(env, assetsManager);
    readObj("models/pyramid.obj");
}

}