//
// Created by Roshan on 12-12-2020.
//
#ifdef __cplusplus
extern "C" {
#endif

#ifndef OPENGL_RENDERER_H
#define OPENGL_RENDERER_H

#include "VertexBuffer.h"
#include "IndexBuffer.h"
#include <android/log.h>

#define ASSERT(x) \
    if(!x) { return; } \

#define clear(x) \
    x; \

#define errors(x) \
    x; \

#define GLCall(x) \
    clear(clearError()); \
    x; \
    errors(checkError(__LINE__,__func__)); \

#define EGLCall(x) \
    x; \
//    errors(checkEGLError(__LINE__,__func__)) \

void clearError();
void checkError(int line, const char *name);
void checkEGLError(int line, const char *name);

#define LOGE(...)  __android_log_print(ANDROID_LOG_ERROR, "Renderer", __VA_ARGS__)


class Renderer {
public:
    void draw(VertexBuffer &va, IndexBuffer &ia);

    void clearBit(int mask);

    void clearColor(float color[4]);
};

#endif //OPENGL_RENDERER_H

#ifdef __cplusplus
}
#endif