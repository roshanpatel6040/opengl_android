//
// Created by Roshan on 12-12-2020.
//

#include "Renderer.h"
#include "IndexBuffer.h"
#include "EGL/egl.h"
#include "GLES2/gl2.h"
#include "GLES2/gl2platform.h"

IndexBuffer::IndexBuffer(unsigned int *data, unsigned int count) : mCount(count) {
    // Size must be same
    ASSERT(sizeof(unsigned int) == sizeof(GLuint))

    GLCall(glGenBuffers(1, &mReferenceID))
    GLCall(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mReferenceID))
    GLCall(glBufferData(GL_ELEMENT_ARRAY_BUFFER, count * sizeof(unsigned int), data,
                        GL_STATIC_DRAW))
}

IndexBuffer::~IndexBuffer() {
    GLCall(glDeleteBuffers(1, &mReferenceID))
}

void IndexBuffer::bind() {
    GLCall(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mReferenceID))
}

void IndexBuffer::unBind() {
    GLCall(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0))
}
