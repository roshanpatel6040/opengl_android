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
    GLCall(shader.unBind())
    GLCall(vb.unBind())
}

/**
 * Draw square
 */
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

    GLCall(Texture texture("/storage/emulated/0/Pictures/UHD Wallpapers/_uhdminimal34.jpg", 0, 4,
                   GL_RGBA))
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

/**
 * Draw Pyramid
 */
void drawPyramid() {
    float vertices[] = {
            0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f,
            -1.0f, 0.0f, -1.0f, 0.0f, 1.0f, 1.0f, 1.0f,
            1.0f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f, 1.0f,
            1.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, 1.0f,
            -1.0f, 0.0f, 1.0f, 0.7f, 0.3f, 0.5f, 1.0f,
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
    unsigned int order[] = {0, 1, 2, 0, 2, 3, 0, 3, 4, 0, 4, 1, 2, 1, 3, 1, 4, 3};
    // RGBA color format
    float color[] = {0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 1.0f};
    // Works from bottom
    glm::mat4 ortho = glm::ortho(-1.0f, (float) 1.0, -1.0f, (float) 1.0, -1.0f,
                                 100.0f);
    glm::mat4 perspective = glm::perspective(glm::radians(
            100.0f), // The vertical Field of View, in radians: the amount of "zoom". Think "camera lens". Usually between 90° (extra wide) and 30° (quite zoomed in)
                                             (float) windowWidth /
                                             (float) windowHeight,       // Aspect Ratio. Depends on the size of your window. Notice that 4/3 == 800/600 == 1280/960, sounds familiar ?
                                             0.1f,              // Near clipping plane. Keep as big as possible, or you'll get precision issues.
                                             50.1f             // Far clipping plane. Keep as little as possible.
    );
    // Translation
    glm::mat4 translation = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 0.0f));
    // Scaling
    glm::mat4 scaleMatrix = glm::scale(glm::mat4(1.0f), glm::vec3(3.0f));
    // Rotation
    glm::mat4 rotationMatrixX = rotate(glm::mat4(1.0f), glm::radians(30.0f),
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
                                 "attribute vec4 color;\n"
                                 "varying vec4 v_color;\n"
                                 //                                 "attribute vec2 texChords;\n"
                                 "uniform mat4 u_MVP;\n"
                                 //                                 "varying vec2 chords;\n"
                                 "void main()\n"
                                 "{\n"
                                 " v_color = color;\n"
                                 " gl_Position = u_MVP * vec4(position,1.0);\n"
                                 //                                 " chords = texChords;\n"
                                 "}";

    string fragmentTextureShader = "uniform vec4 color[2];\n" // Passing multiple rgba
                                   "varying vec4 v_color;\n"
                                   //                                   "uniform sampler2D u_texture;\n"
                                   //                                   "varying vec2 chords;\n"
                                   "void main()\n"
                                   "{\n"
                                   //                                   " vec4 textureColor = texture2D(u_texture,chords);\n"
                                   //                                   " gl_FragColor = textureColor;\n" // assign it to texture color
                                   " gl_FragColor = v_color;\n" // select which color is selected from array
                                   "}";
    GLCall(glEnable(GL_DEPTH_TEST))
    GLCall(GLuint program = glCreateProgram())
    GLCall(Shader shader(program, vertexTextureShader, fragmentTextureShader))
    GLCall(glLinkProgram(program))
    GLCall(VertexBuffer vb(vertices, sizeof(vertices)))
    GLCall(IndexBuffer ib(order, sizeof(order)))
    GLCall(glUseProgram(program))

    GLCall(GLint positionHandle = shader.getAttributeLocation("position"))
    GLCall(shader.vertexAttribPointer(positionHandle, GL_FLOAT, 3, 7 * sizeof(float),
                                      (GLvoid *) nullptr))
    GLCall(shader.enableVertexAttribArray(positionHandle))

    GLCall(GLint colorPositionHandle = shader.getAttributeLocation("color"))
    GLCall(shader.vertexAttribPointer(colorPositionHandle, GL_FLOAT, 4, 7 * sizeof(float),
                                      (GLvoid *) (3 * sizeof(float))))
    GLCall(shader.enableVertexAttribArray(colorPositionHandle))

    GLCall(GLint projectionHandle = shader.getUniformLocation("u_MVP"))
    GLCall(shader.setUniformMatrix4fv(projectionHandle, 1, glm::value_ptr(projection)))

    GLCall(GLint colorHandle = shader.getUniformLocation("color"))
    GLCall(shader.setUniform4fv(colorHandle, 2, color))

//    GLCall(glDrawElements(GL_TRIANGLES, ib.getCount(), GL_UNSIGNED_INT, nullptr))
    GLCall(glDrawArrays(GL_TRIANGLES, 0, ib.getCount()))
    GLCall(shader.disableVertexAttribPointer(positionHandle))
    GLCall(shader.disableVertexAttribPointer(colorPositionHandle))
    GLCall(shader.unBind())
    GLCall(vb.unBind())
    GLCall(ib.unBind())
}

void loadMultipleTriangles() {
    int CHORDS_COLOR_PER_VERTEX = 4;
    int BYTES_PER_FLOAT = 4;
    float vertices[] = {
            // Triangle 1
            //    X    Y     Z     R     G      B    A
            0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f,
            -1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f,
            1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f,
            // Triangle 2
            -1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 1.0f,
            1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f,
            0.0f, -1.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, 1.0f,
    };

    float color[] = {1.0f, 0.5f, 0.0f, 1.0f};

    // varying field uses to transfer data between vertex and fragment shader
    std::string vertexShaderCode = "attribute vec4 position;\n"
                                   "attribute vec4 v_color;\n"
                                   "attribute vec2 texChords;\n"
                                   "attribute float textureFloat;\n"
                                   "varying vec4 f_color;\n" // Passes to fragment shader
                                   "varying float texturePosition;\n"
                                   "varying vec2 chords;\n"
                                   "void main()\n"
                                   "{\n"
                                   " texturePosition = textureFloat;\n"
                                   " chords = texChords;\n"
                                   " f_color = v_color;\n"
                                   " gl_Position = position;\n"
                                   "}";
    std::string fragmentShaderCode = "uniform vec4 color;\n" // Get using uniform location
                                     "varying vec4 f_color;\n" // Get from vertex shader
                                     "varying float texturePosition;\n"
                                     "varying vec2 chords;\n"
                                     "uniform sampler2D textures[2];\n"
                                     "void main()\n"
                                     "{\n"
                                     " highp int index = int(texturePosition);\n"
                                     " gl_FragColor = texture2D(textures[index],chords);\n"
                                     //                                     " gl_FragColor = f_color;\n"
                                     //                                " gl_FragColor = color;\n"
                                     "}";

    GLCall(GLuint program = glCreateProgram())
    GLCall(Shader shader(program, vertexShaderCode, fragmentShaderCode))
    GLCall(glLinkProgram(program))
    GLCall(glUseProgram(program))

    GLCall(VertexBuffer vb(vertices, sizeof(vertices)))

    GLCall(GLint positionHandle = shader.getAttributeLocation("position"))
    GLCall(shader.vertexAttribPointer(positionHandle, GL_FLOAT, 3, 10 * sizeof(float),
                                      (GLvoid *) nullptr))
    GLCall(shader.enableVertexAttribArray(positionHandle))

    GLCall(GLuint colorPositionHandle = shader.getAttributeLocation("v_color"))
    GLCall(shader.vertexAttribPointer(colorPositionHandle, GL_FLOAT, 4, 10 * sizeof(float),
                                      (GLvoid *) (3 * sizeof(float))))
    GLCall(shader.enableVertexAttribArray(colorPositionHandle))

    GLCall(GLuint chordsPositionHandle = shader.getAttributeLocation("texChords"))
    GLCall(shader.vertexAttribPointer(chordsPositionHandle, GL_FLOAT, 2, 10 * sizeof(float),
                                      (GLvoid *) (7 * sizeof(float))))
    GLCall(shader.enableVertexAttribArray(chordsPositionHandle))

    GLCall(GLuint textureIndexPositionHandle = shader.getAttributeLocation("texturePosition"))
    GLCall(shader.vertexAttribPointer(textureIndexPositionHandle, GL_FLOAT, 1, 10 * sizeof(float),
                                      (GLvoid *) (9 * sizeof(float))))
    GLCall(shader.enableVertexAttribArray(textureIndexPositionHandle))

    GLCall(int colorHandle = shader.getUniformLocation("color"))
    GLCall(shader.setUniform4fv(colorHandle,
                                sizeof(color) / CHORDS_COLOR_PER_VERTEX / BYTES_PER_FLOAT, color))

    Texture texture1("/storage/emulated/0/Pictures/UHD Wallpapers/_uhdminimal34.jpg", 1, 4,
                     GL_RGBA);
    Texture texture2("/storage/emulated/0/texture.png", 0, 3, GL_RGB);

    GLint textures[2] = {0, 1};

    GLCall(GLint textureSamplerLocation = shader.getUniformLocation("textures"))
    GLCall(shader.setUniform1iv(textureSamplerLocation, textures))

    GLCall(glDrawArrays(GL_TRIANGLES, 0, 6))
    GLCall(shader.disableVertexAttribPointer(positionHandle))
    GLCall(shader.disableVertexAttribPointer(colorPositionHandle))
    GLCall(shader.disableVertexAttribPointer(textureIndexPositionHandle))
    GLCall(shader.disableVertexAttribPointer(chordsPositionHandle))
    GLCall(shader.unBind())
    GLCall(texture1.unBind())
    GLCall(texture2.unBind())
    GLCall(vb.unBind())
}

void rotateModel() {
    GLCall()
}

extern "C" {
void Java_com_demo_opengl_Render_render(JNIEnv *env, jobject object) {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
//    drawSquare();
//    drawTriangle();
    drawPyramid();
//    loadMultipleTriangles();
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

void
Java_com_demo_opengl_GlSurface_touchPoints(JNIEnv *env, jobject object, jfloat x, jfloat y) {
    rotateModel();
}
}