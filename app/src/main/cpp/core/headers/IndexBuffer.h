//
// Created by Roshan on 12-12-2020.
//
#ifdef __cplusplus
extern "C" {
#endif

#ifndef OPENGL_INDEXBUFFER_H
#define OPENGL_INDEXBUFFER_H

class IndexBuffer {
private:
    unsigned int mReferenceID;
    unsigned int mCount;

public:
    IndexBuffer(unsigned int *data, unsigned int count);

    ~IndexBuffer();

    void bind();

    void unBind();

    inline unsigned int getCount() { return mCount; };
};

#endif //OPENGL_INDEXBUFFER_H

#ifdef __cplusplus
}
#endif