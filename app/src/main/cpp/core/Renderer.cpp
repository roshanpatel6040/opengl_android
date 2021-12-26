//
// Created by Roshan on 12-12-2020.
//

#include "EGL/egl.h"
#include "GLES2/gl2.h"
#include "GLES2/gl2platform.h"
#include <string>
#include "android/log.h"
#include "Renderer.h"
#include "EGL/egl.h"

const char *TAG = "Renderer";

#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, TAG, __VA_ARGS__);
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, TAG, __VA_ARGS__);

void clearError() {
    while (glGetError() != GL_NO_ERROR) {}
}

void checkEGLError(int line, const char *function) {
    EGLint error = eglGetError();
    __android_log_print(ANDROID_LOG_ERROR, "EGL error", "%i function: %s at line %d",
                        error,
                        function,
                        line);
}

void checkError(int line, const char *function) {
    while (GLenum error = glGetError()) {
        switch (error) {
            case GL_INVALID_ENUM:
                __android_log_print(ANDROID_LOG_ERROR, TAG, "%s function: %s at line %d",
                                    "GL_INVALID_ENUM",
                                    function,
                                    line);
                break;
            case GL_INVALID_VALUE:
                __android_log_print(ANDROID_LOG_ERROR, TAG, "%s function: %s at line %d",
                                    "GL_INVALID_VALUE",
                                    function, line);
                break;
            case GL_INVALID_OPERATION:
                __android_log_print(ANDROID_LOG_ERROR, TAG, "%s function: %s at line %d",
                                    "GL_INVALID_OPERATION",
                                    function, line);
                break;
            case GL_OUT_OF_MEMORY:
                __android_log_print(ANDROID_LOG_ERROR, TAG, "%s function: %s at line %d",
                                    "GL_OUT_OF_MEMORY",
                                    function,
                                    line);
                break;
            case GL_INVALID_FRAMEBUFFER_OPERATION:
                __android_log_print(ANDROID_LOG_ERROR, TAG, "%s function: %s at line %d",
                                    "GL_INVALID_FRAMEBUFFER_OPERATION",
                                    function,
                                    line);
                break;
            default:
                __android_log_print(ANDROID_LOG_ERROR, TAG, "At line %i %s %x", line, function,
                                    error);
                break;
        }
    }
}

void Renderer::draw(VertexBuffer &va, IndexBuffer &ia) {
    va.bind();
    ia.bind();
    GLCall(glDrawElements(GL_TRIANGLES, ia.getCount(), GL_UNSIGNED_INT, nullptr))
}

void Renderer::clearBit(int mask) {
    GLCall(glClear(mask))
}

void Renderer::clearColor(float *color) {
    GLCall(glClearColor(color[0], color[1], color[2], color[3]))
}
