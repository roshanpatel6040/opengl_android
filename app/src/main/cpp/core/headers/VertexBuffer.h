//
// Created by Roshan on 12-12-2020.
//
#ifdef __cplusplus
extern "C" {
#endif

#ifndef OPENGL_VERTEXBUFFER_H
#define OPENGL_VERTEXBUFFER_H

class VertexBuffer {
private:
    unsigned int mReferenceID;

public:
    VertexBuffer(const void *data, unsigned int size);

    ~VertexBuffer();

    void bind();

    void unBind();
};

#endif //OPENGL_VERTEXBUFFER_H

#ifdef __cplusplus
}
#endif