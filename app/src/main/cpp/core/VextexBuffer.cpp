//
// Created by Roshan on 12-12-2020.
//

#include "VertexBuffer.h"
#include "Renderer.h"
#include "EGL/egl.h"
#include "GLES2/gl2.h"
#include "GLES2/gl2platform.h"

VertexBuffer::VertexBuffer() {

}

VertexBuffer::VertexBuffer(const void *data, unsigned int size) {
    glGenBuffers(1, &mReferenceID);
    glBindBuffer(GL_ARRAY_BUFFER, mReferenceID);
    glBufferData(GL_ARRAY_BUFFER, size, data, GL_STATIC_DRAW);
}

void VertexBuffer::bind() {
    glBindBuffer(GL_ARRAY_BUFFER, mReferenceID);
}

void VertexBuffer::unBind() {
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

VertexBuffer::~VertexBuffer() {
    glDeleteBuffers(1, &mReferenceID);
}


