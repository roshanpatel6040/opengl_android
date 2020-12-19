//
// Created by Roshan on 12-12-2020.
//
#include <string>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef OPENGL_TEXTURE_H
#define OPENGL_TEXTURE_H

class Texture {
private:
    unsigned int mReferenceID;
    unsigned int slot;
    int width = 0;
    int height = 0;
    int bpp;
    unsigned char *allocationBuffer;

public:
    Texture(std::string path, int slot, int channel, int type);

    void bind();

    void unBind();

    ~Texture();

    inline int getSlot() { return slot; };

    inline int getReferenceId() { return mReferenceID; };
};

#endif //OPENGL_TEXTURE_H

#ifdef __cplusplus
}
#endif